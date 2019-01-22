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

#include <buildboxcommon_client.h>

#include <errno.h>
#include <fstream>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>

namespace buildboxcommon {

#define BUILDBOXCOMMON_CLIENT_BUFFER_SIZE (1024 * 1024)

namespace {
static std::string getFileContents(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::string("Failed to open file ") +
                                 filename);
    }

    std::string contents;

    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());

    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    if (!in) {
        in.close();
        throw std::runtime_error(std::string("Failed to read file ") +
                                 filename);
    }

    in.close();
    return contents;
}
} // namespace

void Client::init(const char *remoteUrl, const char *serverCert,
                  const char *clientKey, const char *clientCert)
{
    std::string target;
    std::shared_ptr<grpc::ChannelCredentials> creds;
    if (strncmp(remoteUrl, "http://", strlen("http://")) == 0) {
        target = remoteUrl + strlen("http://");
        creds = grpc::InsecureChannelCredentials();
    }
    else if (strncmp(remoteUrl, "https://", strlen("https://")) == 0) {
        auto options = grpc::SslCredentialsOptions();
        if (serverCert) {
            options.pem_root_certs = getFileContents(serverCert);
        }
        if (clientKey) {
            options.pem_private_key = getFileContents(clientKey);
        }
        if (clientCert) {
            options.pem_cert_chain = getFileContents(clientCert);
        }
        target = remoteUrl + strlen("https://");
        creds = grpc::SslCredentials(options);
    }
    else {
        throw std::runtime_error("Unsupported URL scheme");
    }

    this->d_channel = grpc::CreateChannel(target, creds);
    this->d_bytestreamClient = ByteStream::NewStub(this->d_channel);
    this->d_casClient = ContentAddressableStorage::NewStub(this->d_channel);
    this->d_capabilitiesClient = Capabilities::NewStub(this->d_channel);

    // initialise update/read sizes
    this->d_batchUpdateSize = 0;
    this->d_batchReadSize = 0;

    // The default limit for gRPC messages is 4 MiB.
    // Limit payload to 1 MiB to leave sufficient headroom for metadata.
    this->d_maxBatchTotalSizeBytes = BUILDBOXCOMMON_CLIENT_BUFFER_SIZE;

    grpc::ClientContext context;
    GetCapabilitiesRequest request;
    ServerCapabilities response;
    auto status = this->d_capabilitiesClient->GetCapabilities(
        &context, request, &response);
    if (status.ok()) {
        int64_t serverMaxBatchTotalSizeBytes =
            response.cache_capabilities().max_batch_total_size_bytes();
        // 0 means no server limit
        if (serverMaxBatchTotalSizeBytes > 0 &&
            serverMaxBatchTotalSizeBytes < this->d_maxBatchTotalSizeBytes) {
            this->d_maxBatchTotalSizeBytes = serverMaxBatchTotalSizeBytes;
        }
    }

    // Generate UUID to use for uploads
    uuid_t uu;
    uuid_generate(uu);
    this->d_uuid = std::string(36, 0);
    uuid_unparse_lower(uu, &this->d_uuid[0]);
}

void Client::download(int fd, const Digest &digest)
{
    std::string resourceName;
    resourceName.append("blobs/");
    resourceName.append(digest.hash());
    resourceName.append("/");
    resourceName.append(std::to_string(digest.size_bytes()));

    grpc::ClientContext context;
    ReadRequest request;
    request.set_resource_name(resourceName);
    request.set_read_offset(0);
    auto reader = this->d_bytestreamClient->Read(&context, request);
    ReadResponse response;
    while (reader->Read(&response)) {
        if (write(fd, response.data().c_str(), response.data().length()) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    struct stat st;
    fstat(fd, &st);
    if (st.st_size != digest.size_bytes()) {
        throw std::runtime_error(
            "Size of downloaded blob does not match digest");
    }
}

void Client::upload(int fd, const Digest &digest)
{
    std::vector<char> buffer(BUILDBOXCOMMON_CLIENT_BUFFER_SIZE);

    std::string resourceName;
    resourceName.append("uploads/");
    resourceName.append(this->d_uuid);
    resourceName.append("/blobs/");
    resourceName.append(digest.hash());
    resourceName.append("/");
    resourceName.append(std::to_string(digest.size_bytes()));

    lseek(fd, 0, SEEK_SET);

    grpc::ClientContext context;
    WriteResponse response;
    auto writer = this->d_bytestreamClient->Write(&context, &response);
    ssize_t offset = 0;
    bool lastChunk = false;
    while (!lastChunk) {
        ssize_t bytesRead =
            read(fd, &buffer[0], BUILDBOXCOMMON_CLIENT_BUFFER_SIZE);
        if (bytesRead < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        WriteRequest request;
        request.set_resource_name(resourceName);
        request.set_write_offset(offset);
        request.set_data(&buffer[0], bytesRead);

        if (offset + bytesRead < digest.size_bytes()) {
            if (bytesRead == 0) {
                throw std::runtime_error("Upload of " + digest.hash() +
                                         " failed: unexpected end of file");
            }
        }
        else {
            lastChunk = true;
            request.set_finish_write(true);
        }

        if (!writer->Write(request)) {
            throw std::runtime_error("Upload of " + digest.hash() +
                                     " failed: broken stream");
        }

        offset += bytesRead;
    }

    writer->WritesDone();
    auto status = writer->Finish();
    if (!status.ok() || offset != digest.size_bytes()) {
        throw std::runtime_error("Upload of " + digest.hash() + " failed");
    }
}

bool Client::batchUploadAdd(const Digest &digest,
                            const std::vector<char> &data)
{
    // check if batch size has got too large
    int64_t newBatchSize = this->d_batchUpdateSize + digest.size_bytes();
    if (newBatchSize > this->d_maxBatchTotalSizeBytes) {
        return false;
    }

    // create and add BatchUpdateBlob request
    BatchUpdateBlobsRequest_Request request =
        BatchUpdateBlobsRequest_Request();
    request.mutable_digest()->CopyFrom(digest);
    std::string d(data.begin(), data.begin() + digest.size_bytes());
    request.set_data(d);
    this->d_batchUpdateRequest.add_requests()->CopyFrom(request);
    this->d_batchUpdateSize = newBatchSize;

    return true;
}

bool Client::batchUpload()
{
    grpc::ClientContext context;
    auto status = this->d_casClient->BatchUpdateBlobs(
        &context, this->d_batchUpdateRequest, &this->d_batchUpdateResponse);
    if (!status.ok()) {
        throw std::runtime_error("Batch upload failed");
    }

    for (auto i : this->d_batchUpdateResponse.responses()) {
        if (i.status().code() != GRPC_STATUS_OK) {
            throw std::runtime_error("Batch upload failed");
        }
    }

    this->d_batchUpdateRequest.Clear();
    this->d_batchUpdateSize = 0;
    return true;
}

bool Client::batchDownloadAdd(const Digest &digest)
{
    assert(!this->d_batchReadRequestSent);

    int64_t newBatchSize = this->d_batchReadSize + digest.size_bytes();
    if (newBatchSize > this->d_maxBatchTotalSizeBytes) {
        // Not enough space left in current batch
        return false;
    }

    this->d_batchReadRequest.add_digests()->CopyFrom(digest);
    this->d_batchReadSize = newBatchSize;

    return true;
}

bool Client::batchDownloadNext(const Digest **digest, const std::string **data)
{
    if (!this->d_batchReadRequestSent) {
        if (this->d_batchReadRequest.digests_size() == 0) {
            // Empty batch
            return false;
        }

        this->d_batchReadContext = std::make_unique<grpc::ClientContext>();
        auto status = this->d_casClient->BatchReadBlobs(
            this->d_batchReadContext.get(), this->d_batchReadRequest,
            &this->d_batchReadResponse);
        if (!status.ok()) {
            throw std::runtime_error("Batch download failed");
        }
        this->d_batchReadRequestSent = true;
        this->d_batchReadResponseIndex = 0;
    }

    if (this->d_batchReadResponseIndex >=
        this->d_batchReadResponse.responses_size()) {
        // End of batch
        this->d_batchReadContext = nullptr;
        this->d_batchReadRequest.Clear();
        this->d_batchReadResponse.Clear();
        this->d_batchReadRequestSent = false;
        this->d_batchReadResponseIndex = 0;
        this->d_batchReadSize = 0;
        return false;
    }

    // Return next entry
    this->d_batchReadBlobResponse =
        this->d_batchReadResponse.responses(this->d_batchReadResponseIndex);

    if (this->d_batchReadBlobResponse.status().code() != GRPC_STATUS_OK) {
        throw std::runtime_error("Batch download failed");
    }

    *digest = &this->d_batchReadBlobResponse.digest();
    *data = &this->d_batchReadBlobResponse.data();

    this->d_batchReadResponseIndex++;
    return true;
}
} // namespace buildboxcommon
