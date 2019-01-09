/*
 * Copyright 2018 Bloomberg Finance LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <uuid/uuid.h>

#include "buildbox-common.h"

static std::string get_file_contents(const char *filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in) {
		throw std::runtime_error(std::string("Failed to open file ") + filename);
	}

	std::string contents;

	in.seekg(0, std::ios::end);
	contents.resize(in.tellg());

	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	if (!in) {
		in.close();
		throw std::runtime_error(std::string("Failed to read file ") + filename);
	}

	in.close();
	return contents;
}

void Client::init(const char *remote_url, const char *server_cert, const char *client_key, const char *client_cert)
{
	std::string target;
	std::shared_ptr<grpc::ChannelCredentials> creds;
	if (strncmp(remote_url, "http://", strlen("http://")) == 0) {
		target = remote_url + strlen("http://");
		creds = grpc::InsecureChannelCredentials();
	} else if (strncmp(remote_url, "https://", strlen("https://")) == 0) {
		auto options = grpc::SslCredentialsOptions();
		if (server_cert) {
			options.pem_root_certs = get_file_contents(server_cert);
		}
		if (client_key) {
			options.pem_private_key = get_file_contents(client_key);
		}
		if (client_cert) {
			options.pem_cert_chain = get_file_contents(client_cert);
		}
		target = remote_url + strlen("https://");
		creds = grpc::SslCredentials(options);
	} else {
		throw std::runtime_error("Unsupported URL scheme");
	}

	this->channel = grpc::CreateChannel(target, creds);
	this->bytestream_client = ByteStream::NewStub(this->channel);
	this->cas_client = ContentAddressableStorage::NewStub(this->channel);
	this->capabilities_client = Capabilities::NewStub(this->channel);

	// initialise update/read sizes
	this->batch_update_size = 0;
	this->batch_read_size = 0;

	/* The default limit for gRPC messages is 4 MiB.
	 * Limit payload to 1 MiB to leave sufficient headroom for metadata. */
	this->max_batch_total_size_bytes = BUFFER_SIZE;

	grpc::ClientContext context;
	GetCapabilitiesRequest request;
	ServerCapabilities response;
	auto status = this->capabilities_client->GetCapabilities(&context, request, &response);
	if (status.ok()) {
		int64_t server_max_batch_total_size_bytes = response.cache_capabilities().max_batch_total_size_bytes();
		/* 0 means no server limit */
		if (server_max_batch_total_size_bytes > 0 &&
		    server_max_batch_total_size_bytes < this->max_batch_total_size_bytes) {
			this->max_batch_total_size_bytes = server_max_batch_total_size_bytes;
		}
	}

	/* Generate UUID to use for uploads */
	uuid_t uu;
	uuid_generate(uu);
	this->uuid = std::string(36, 0);
	uuid_unparse_lower(uu, &this->uuid[0]);
}

void Client::download(int fd, const Digest& digest)
{
	std::string resource_name;
	resource_name.append("blobs/");
	resource_name.append(digest.hash());
	resource_name.append("/");
	resource_name.append(std::to_string(digest.size_bytes()));

	grpc::ClientContext context;
	ReadRequest request;
	request.set_resource_name(resource_name);
	request.set_read_offset(0);
	auto reader = this->bytestream_client->Read(&context, request);
	ReadResponse response;
	while (reader->Read(&response)) {
		if (write(fd, response.data().c_str(), response.data().length()) < 0) {
			throw std::system_error(errno, std::generic_category());
		}
	}

	struct stat st;
	fstat(fd, &st);
	if (st.st_size != digest.size_bytes()) {
		throw std::runtime_error("Size of downloaded blob does not match digest");
	}
}

void Client::upload(int fd, const Digest& digest)
{
	std::vector<char> buffer(BUFFER_SIZE);

	std::string resource_name;
	resource_name.append("uploads/");
	resource_name.append(this->uuid);
	resource_name.append("/blobs/");
	resource_name.append(digest.hash());
	resource_name.append("/");
	resource_name.append(std::to_string(digest.size_bytes()));

	lseek(fd, 0, SEEK_SET);

	grpc::ClientContext context;
	WriteResponse response;
	auto writer = this->bytestream_client->Write(&context, &response);
	ssize_t offset = 0;
	bool last_chunk = false;
	while (!last_chunk) {
		ssize_t bytes_read = read(fd, &buffer[0], BUFFER_SIZE);
		if (bytes_read < 0) {
			throw std::system_error(errno, std::generic_category());
		}

		WriteRequest request;
		request.set_resource_name(resource_name);
		request.set_write_offset(offset);
		request.set_data(&buffer[0], bytes_read);

		if (offset + bytes_read < digest.size_bytes()) {
			if (bytes_read == 0) {
				throw std::runtime_error("Upload of " + digest.hash() + " failed: unexpected end of file");
			}
		} else {
			last_chunk = true;
			request.set_finish_write(true);
		}

		if (!writer->Write(request)) {
			throw std::runtime_error("Upload of " + digest.hash() + " failed: broken stream");
		}

		offset += bytes_read;
	}

	writer->WritesDone();
	auto status = writer->Finish();
	if (!status.ok() || offset != digest.size_bytes()) {
		throw std::runtime_error("Upload of " + digest.hash() + " failed");
	}
}

bool Client::batch_upload_add(const Digest& digest, const std::vector<char>& data)
{
	// check if batch size has got too large
	int64_t new_batch_size = this->batch_update_size + digest.size_bytes();
	if (new_batch_size > this->max_batch_total_size_bytes) {
		return false;
	}

	// create and add BatchUpdateBlob request
	BatchUpdateBlobsRequest_Request request = BatchUpdateBlobsRequest_Request();
	request.mutable_digest()->CopyFrom(digest);
	std::string d(data.begin(), data.begin() + digest.size_bytes());
	request.set_data(d);
	this->batch_update_request.add_requests()->CopyFrom(request);
	this->batch_update_size = new_batch_size;

	return true;
}

bool Client::batch_upload()
{
	grpc::ClientContext context;
	auto status = this->cas_client->BatchUpdateBlobs(
		&context,
		this->batch_update_request,
		&this->batch_update_response);
	if (!status.ok()) {
		throw std::runtime_error("Batch upload failed");
	}

	for (auto i : this->batch_update_response.responses()) {
		if (i.status().code() != GRPC_STATUS_OK) {
			throw std::runtime_error("Batch upload failed");
		}
	}

	this->batch_update_request.Clear();
	this->batch_update_size = 0;
	return true;
}

bool Client::batch_download_add(const Digest& digest)
{
	assert(!this->batch_read_request_sent);

	int64_t new_batch_size = this->batch_read_size + digest.size_bytes();
	if (new_batch_size > this->max_batch_total_size_bytes) {
		/* Not enough space left in current batch */
		return false;
	}

	this->batch_read_request.add_digests()->CopyFrom(digest);
	this->batch_read_size = new_batch_size;

	return true;
}

bool Client::batch_download_next(const Digest **digest, const std::string **data)
{
	if (!this->batch_read_request_sent) {
		if (this->batch_read_request.digests_size() == 0) {
			/* Empty batch */
			return false;
		}

		this->batch_read_context = std::make_unique<grpc::ClientContext>();
		auto status = this->cas_client->BatchReadBlobs(this->batch_read_context.get(), this->batch_read_request, &this->batch_read_response);
		if (!status.ok()) {
			throw std::runtime_error("Batch download failed");
		}
		this->batch_read_request_sent = true;
		this->batch_read_response_index = 0;
	}

	if (this->batch_read_response_index >= this->batch_read_response.responses_size()) {
		/* End of batch */
		this->batch_read_context = nullptr;
		this->batch_read_request.Clear();
		this->batch_read_response.Clear();
		this->batch_read_request_sent = false;
		this->batch_read_response_index = 0;
		this->batch_read_size = 0;
		return false;
	}

	/* Return next entry */
	this->batch_read_blob_response = this->batch_read_response.responses(this->batch_read_response_index);

	if (this->batch_read_blob_response.status().code() != GRPC_STATUS_OK) {
		throw std::runtime_error("Batch download failed");
	}

	*digest = &this->batch_read_blob_response.digest();
	*data = &this->batch_read_blob_response.data();

	this->batch_read_response_index++;
	return true;
}
