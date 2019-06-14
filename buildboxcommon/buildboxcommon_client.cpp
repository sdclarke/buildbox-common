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
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_merklize.h>

#include <algorithm>
#include <errno.h>
#include <fstream>
#include <grpc/grpc.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>

namespace buildboxcommon {

#define BYTESTREAM_CHUNK_SIZE (1024 * 1024)

void Client::init(const ConnectionOptions &options)
{
    std::shared_ptr<grpc::Channel> channel = options.createChannel();
    this->d_grpcRetryLimit = std::stoi(options.d_retryLimit);
    this->d_grpcRetryDelay = std::stoi(options.d_retryDelay);
    this->d_channel = channel;

    if (options.d_instanceName != nullptr) {
        this->d_instanceName = std::string(options.d_instanceName);
    }

    std::shared_ptr<ByteStream::Stub> bytestreamClient =
        ByteStream::NewStub(this->d_channel);
    std::shared_ptr<ContentAddressableStorage::Stub> casClient =
        ContentAddressableStorage::NewStub(this->d_channel);
    std::shared_ptr<Capabilities::Stub> capabilitiesClient =
        Capabilities::NewStub(this->d_channel);
    init(bytestreamClient, casClient, capabilitiesClient);
}

void Client::init(
    std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
    std::shared_ptr<ContentAddressableStorage::StubInterface> casClient,
    std::shared_ptr<Capabilities::StubInterface> capabilitiesClient)
{
    this->d_bytestreamClient = bytestreamClient;
    this->d_casClient = casClient;
    this->d_capabilitiesClient = capabilitiesClient;

    // The default limit for gRPC messages is 4 MiB.
    // Limit payload to 1 MiB to leave sufficient headroom for metadata.
    this->d_maxBatchTotalSizeBytes = BYTESTREAM_CHUNK_SIZE;

    auto getCapabilitiesLambda = [&](grpc::ClientContext &context) {
        GetCapabilitiesRequest request;
        request.set_instance_name(d_instanceName);

        ServerCapabilities response;
        auto status = this->d_capabilitiesClient->GetCapabilities(
            &context, request, &response);
        if (status.ok()) {
            int64_t serverMaxBatchTotalSizeBytes =
                response.cache_capabilities().max_batch_total_size_bytes();
            // 0 means no server limit
            if (serverMaxBatchTotalSizeBytes > 0 &&
                serverMaxBatchTotalSizeBytes <
                    this->d_maxBatchTotalSizeBytes) {
                this->d_maxBatchTotalSizeBytes = serverMaxBatchTotalSizeBytes;
            }
        }
        return status;
    };

    try {
        grpcRetry(getCapabilitiesLambda, this->d_grpcRetryLimit,
                  this->d_grpcRetryDelay);
    }

    catch (const std::runtime_error &e) {
        BUILDBOX_LOG_DEBUG("Get capabilities request failed. Using default. "
                           << e.what());
    }

    // Generate UUID to use for uploads
    uuid_t uu;
    uuid_generate(uu);
    this->d_uuid = std::string(36, 0);
    uuid_unparse_lower(uu, &this->d_uuid[0]);
}

std::string Client::makeResourceName(const Digest &digest, bool isUpload)
{
    std::string resourceName;

    if (!d_instanceName.empty()) {
        resourceName.append(d_instanceName);
        resourceName.append("/");
    }

    if (isUpload) {
        resourceName.append("uploads/");
        resourceName.append(this->d_uuid);
        resourceName.append("/");
    }

    resourceName.append("blobs/");
    resourceName.append(digest.hash());
    resourceName.append("/");
    resourceName.append(std::to_string(digest.size_bytes()));
    return resourceName;
}

std::string Client::fetchString(const Digest &digest)
{
    BUILDBOX_LOG_DEBUG("Downloading " << digest.hash() << " to string");
    std::string resourceName = this->makeResourceName(digest, false);

    std::string result;

    auto fetchLambda = [&](grpc::ClientContext &context) {
        ReadRequest request;
        request.set_resource_name(resourceName);
        request.set_read_offset(0);
        auto reader = this->d_bytestreamClient->Read(&context, request);
        ReadResponse response;
        while (reader->Read(&response)) {
            result += response.data();
        }

        const auto read_status = reader->Finish();
        if (!read_status.ok()) {
            throw std::runtime_error("Error fetching string: " +
                                     read_status.error_message());
        }

        if ((int)result.length() != digest.size_bytes()) {
            std::stringstream errorMsg;
            errorMsg << "Expected " << digest.size_bytes()
                     << " bytes, but downloaded blob was " << result.length()
                     << " bytes";
            throw std::runtime_error(errorMsg.str());
        }

        BUILDBOX_LOG_DEBUG(resourceName << ": " << result.length()
                                        << " bytes retrieved");
        return read_status;
    };

    grpcRetry(fetchLambda, this->d_grpcRetryLimit, this->d_grpcRetryDelay);
    return result;
}

void Client::download(int fd, const Digest &digest)
{
    BUILDBOX_LOG_DEBUG("Downloading " << digest.hash() << " to file");
    std::string resourceName = this->makeResourceName(digest, false);

    auto downloadLambda = [&](grpc::ClientContext &context) {
        ReadRequest request;
        request.set_resource_name(resourceName);
        request.set_read_offset(0);
        auto reader = this->d_bytestreamClient->Read(&context, request);
        ReadResponse response;
        while (reader->Read(&response)) {
            if (write(fd, response.data().c_str(), response.data().length()) <
                0) {
                throw std::system_error(errno, std::generic_category());
            }
        }

        const auto read_status = reader->Finish();
        if (!read_status.ok()) {
            throw std::runtime_error("Error downloading blob: " +
                                     read_status.error_message());
        }

        struct stat st;
        fstat(fd, &st);
        if (st.st_size != digest.size_bytes()) {
            std::stringstream errorMsg;
            errorMsg << "Expected " << digest.size_bytes()
                     << " bytes, but downloaded blob was " << st.st_size
                     << " bytes";
            throw std::runtime_error(errorMsg.str());
        }
        BUILDBOX_LOG_DEBUG(resourceName << ": " << st.st_size
                                        << " bytes retrieved");
        return read_status;
    };
    grpcRetry(downloadLambda, this->d_grpcRetryLimit, this->d_grpcRetryDelay);
}

void Client::upload(const std::string &str, const Digest &digest)
{
    BUILDBOX_LOG_DEBUG("Uploading " << digest.hash() << " from string");
    if (digest.size_bytes() != (int)str.length()) {
        std::stringstream errorMsg;
        errorMsg << "Digest length of " << digest.size_bytes() << " bytes for "
                 << digest.hash() << " does not match string length of "
                 << str.length() << " bytes";
        throw std::logic_error(errorMsg.str());
    }
    std::string resourceName = this->makeResourceName(digest, true);

    auto uploadLambda = [&](grpc::ClientContext &context) {
        WriteResponse response;
        auto writer = this->d_bytestreamClient->Write(&context, &response);
        ssize_t offset = 0;
        bool lastChunk = false;
        while (!lastChunk) {
            WriteRequest request;
            request.set_resource_name(resourceName);
            request.set_write_offset(offset);

            int uploadLength =
                std::min(static_cast<unsigned long>(BYTESTREAM_CHUNK_SIZE),
                         str.length() - offset);

            request.set_data(&str[offset], uploadLength);
            offset += uploadLength;

            lastChunk = (offset == (ssize_t)str.length());
            if (lastChunk) {
                request.set_finish_write(lastChunk);
            }

            if (!writer->Write(request)) {
                throw std::runtime_error("Upload of " + digest.hash() +
                                         " failed: broken stream");
            }
        }
        writer->WritesDone();
        auto status = writer->Finish();
        if (offset != digest.size_bytes()) {
            std::stringstream errorMsg;
            errorMsg << "Expected to upload " << digest.size_bytes()
                     << " bytes for " << digest.hash()
                     << ", but uploaded blob was " << offset << " bytes";
            throw std::runtime_error(errorMsg.str());
        }
        BUILDBOX_LOG_DEBUG(resourceName << ": " << offset
                                        << " bytes uploaded");
        return status;
    };
    grpcRetry(uploadLambda, this->d_grpcRetryLimit, this->d_grpcRetryDelay);
}

void Client::upload(int fd, const Digest &digest)
{
    std::vector<char> buffer(BYTESTREAM_CHUNK_SIZE);
    BUILDBOX_LOG_DEBUG("Uploading " << digest.hash() << " from file");

    std::string resourceName = this->makeResourceName(digest, true);

    lseek(fd, 0, SEEK_SET);

    auto uploadLambda = [&](grpc::ClientContext &context) {
        WriteResponse response;
        auto writer = this->d_bytestreamClient->Write(&context, &response);
        ssize_t offset = 0;
        bool lastChunk = false;
        while (!lastChunk) {
            ssize_t bytesRead = read(fd, &buffer[0], BYTESTREAM_CHUNK_SIZE);
            if (bytesRead < 0) {
                throw std::system_error(errno, std::generic_category());
            }

            WriteRequest request;
            request.set_resource_name(resourceName);
            request.set_write_offset(offset);
            request.set_data(&buffer[0], bytesRead);

            if (offset + bytesRead < digest.size_bytes()) {
                if (bytesRead == 0) {
                    throw std::runtime_error(
                        "Upload of " + digest.hash() +
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
        if (offset != digest.size_bytes()) {
            throw std::runtime_error("Upload of " + digest.hash() + " failed");
        }
        BUILDBOX_LOG_DEBUG(resourceName << ": " << offset
                                        << " bytes uploaded");
        return status;
    };
    grpcRetry(uploadLambda, this->d_grpcRetryLimit, this->d_grpcRetryDelay);
}

Client::UploadResults
Client::uploadBlobs(const std::vector<UploadRequest> &requests)
{
    UploadResults results;

    // We first sort the requests by their sizes in ascending order, so that
    // we can then iterate through that result greedily trying to add as many
    // digests as possible to each request.
    std::vector<UploadRequest> request_list(requests);
    std::sort(request_list.begin(), request_list.end(),
              [](const UploadRequest &r1, const UploadRequest &r2) {
                  return r1.digest.size_bytes() < r2.digest.size_bytes();
              });

    // Grouping the requests into batches (we only need to look at the Digests
    // for their sizes):
    std::vector<Digest> digests;
    for (const auto &r : request_list) {
        digests.push_back(r.digest);
    }

    const auto batches = makeBatches(digests);
    for (const auto &batch_range : batches) {
        const size_t batch_start = batch_range.first;
        const size_t batch_end = batch_range.second;

        const std::vector<Digest> digests_not_uploaded =
            batchUpload(request_list, batch_start, batch_end);
        for (const auto &digest : digests_not_uploaded) {
            results.push_back(digest);
        }
    }

    // Fetching all those digests that might need to be uploaded using the
    // Bytestream API. Those will be in the range [batch_end, batches.size()).
    size_t batch_end;
    if (batches.empty()) {
        batch_end = 0;
    }
    else {
        batch_end = batches.rbegin()->second;
    }

    for (auto d = batch_end; d < request_list.size(); d++) {
        try {
            upload(request_list[d].data, request_list[d].digest);
        }
        catch (const std::runtime_error &) {
            results.push_back(request_list[d].digest);
        }
    }

    return results;
}

Client::DownloadedData
Client::downloadBlobs(const std::vector<Digest> &digests)
{
    DownloadedData downloaded_data;

    // We first sort the digests by their sizes in ascending order, so that
    // we can then iterate through that result greedily trying to add as many
    // digests as possible to each request.
    auto request_list(digests);
    std::sort(request_list.begin(), request_list.end(),
              [](const Digest &d1, const Digest &d2) {
                  return d1.size_bytes() < d2.size_bytes();
              });

    const auto batches = makeBatches(request_list);
    for (const auto &batch_range : batches) {
        const size_t batch_start = batch_range.first;
        const size_t batch_end = batch_range.second;

        // For each batch, we make the request and then store the successfully
        // read data in the result:
        try {
            const auto download_results =
                batchDownload(request_list, batch_start, batch_end);

            for (const auto &download_result : download_results) {
                const std::string digest_hash = download_result.first;
                const std::string data = download_result.second;
                downloaded_data[digest_hash] = data;
            }
        }
        catch (const std::runtime_error &e) {
            std::cerr << "Batch download failed: " + std::string(e.what())
                      << std::endl;
        }
    }

    // Fetching all those digests that might need to be downloaded using the
    // Bytestream API. Those will be in the range [batch_end, batches.size()).
    size_t batch_end;
    if (batches.empty()) {
        batch_end = 0;
    }
    else {
        batch_end = batches.rbegin()->second;
    }

    for (auto d = batch_end; d < request_list.size(); d++) {
        const Digest digest = request_list[d];
        try {
            const auto data = fetchString(digest);
            downloaded_data[digest.hash()] = data;
        }
        catch (const std::runtime_error &e) {
            std::cerr << "Error: fetchString(): " + std::string(e.what())
                      << std::endl;
        }
    }

    return downloaded_data;
}

Client::UploadResults
Client::batchUpload(const std::vector<UploadRequest> &requests,
                    const size_t start_index, const size_t end_index)
{
    assert(start_index <= end_index);
    assert(end_index <= requests.size());

    BatchUpdateBlobsRequest request;
    request.set_instance_name(d_instanceName);

    for (auto d = start_index; d < end_index; d++) {
        auto entry = request.add_requests();
        entry->mutable_digest()->CopyFrom(requests[d].digest);
        entry->set_data(requests[d].data);
    }

    BatchUpdateBlobsResponse response;
    auto batchUploadLamda = [&](grpc::ClientContext &context) {
        const auto status =
            this->d_casClient->BatchUpdateBlobs(&context, request, &response);
        return status;
    };

    grpcRetry(batchUploadLamda, this->d_grpcRetryLimit,
              this->d_grpcRetryDelay);

    UploadResults results;
    for (const auto &uploadResponse : response.responses()) {
        if (uploadResponse.status().code() != GRPC_STATUS_OK) {
            results.push_back(uploadResponse.digest());
        }
    }
    return results;
}

Client::DownloadedData Client::batchDownload(const std::vector<Digest> digests,
                                             const size_t start_index,
                                             const size_t end_index)
{
    assert(start_index <= end_index);
    assert(end_index <= digests.size());

    BatchReadBlobsRequest request;
    request.set_instance_name(d_instanceName);

    for (auto d = start_index; d < end_index; d++) {
        auto digest = request.add_digests();
        digest->CopyFrom(digests[d]);
    }

    BatchReadBlobsResponse response;
    auto batchDownloadLamda = [&](grpc::ClientContext &context) {
        const auto status =
            this->d_casClient->BatchReadBlobs(&context, request, &response);
        return status;
    };

    grpcRetry(batchDownloadLamda, this->d_grpcRetryLimit,
              this->d_grpcRetryDelay);

    DownloadedData data;
    for (const auto &downloadResponse : response.responses()) {
        if (downloadResponse.status().code() == GRPC_STATUS_OK) {
            data[downloadResponse.digest().hash()] = downloadResponse.data();
        }
    }

    return data;
}

std::vector<std::pair<size_t, size_t>>
Client::makeBatches(const std::vector<Digest> &digests)
{
    std::vector<std::pair<size_t, size_t>> batches;

    const auto max_batch_size = d_maxBatchTotalSizeBytes;

    size_t batch_start = 0;
    size_t batch_end = 0;
    // A batch is contains digests inside [batch_start, batch_end).

    while (batch_end < digests.size()) {
        auto bytes_in_batch = 0;
        if (digests[batch_end].size_bytes() > max_batch_size) {
            // All digests from `batch_end` to the end of the list are
            // larger than what we can request; stop.
            return batches;
        }

        // Adding all the digests that we can until we run out or exceed the
        // batch request limit...
        while (batch_end < digests.size() &&
               bytes_in_batch + digests[batch_end].size_bytes() <=
                   max_batch_size) {
            bytes_in_batch += digests[batch_end].size_bytes();
            batch_end++;
        }

        batches.push_back(std::make_pair(batch_start, batch_end));

        batch_start = batch_end;
        batch_end = batch_start;
        bytes_in_batch = 0;
    };

    return batches;
}

std::vector<Digest>
Client::findMissingBlobs(const std::vector<Digest> &digests)
{
    grpc::ClientContext context;

    FindMissingBlobsRequest request;
    request.set_instance_name(d_instanceName);

    for (const auto &digest : digests) {
        auto entry = request.add_blob_digests();
        entry->CopyFrom(digest);
    }

    FindMissingBlobsResponse response;
    const auto status =
        this->d_casClient->FindMissingBlobs(&context, request, &response);

    if (!status.ok()) {
        throw std::runtime_error("FindMissingBlobs() request failed.");
    }

    std::vector<Digest> missing_blobs;
    for (const auto &digest : response.missing_blob_digests()) {
        missing_blobs.push_back(digest);
    }
    return missing_blobs;
}

Client::UploadResults Client::uploadDirectory(const std::string &path,
                                              Digest *root_directory_digest)
{
    // Recursing through the directory and building a map:
    digest_string_map directory_map;
    const NestedDirectory nested_dir =
        make_nesteddirectory(path.c_str(), &directory_map);

    const Digest directory_digest = nested_dir.to_digest(&directory_map);
    if (root_directory_digest != nullptr) {
        root_directory_digest->CopyFrom(directory_digest);
    }

    // FindMissingBlobs():
    directory_map = missingDigests(directory_map);

    // Batch upload the blobs missing in the remote:
    std::vector<UploadRequest> upload_requests;
    upload_requests.reserve(directory_map.size());
    for (const auto &entry : directory_map) {
        const Digest digest = entry.first;

        Directory directory;
        if (directory.ParseFromString(entry.second)) {
            upload_requests.emplace_back(UploadRequest(digest, entry.second));
        }
        else {
            const std::string file_contents =
                FileUtils::get_file_contents(entry.second.c_str());
            upload_requests.emplace_back(UploadRequest(digest, file_contents));
        }
    }

    return uploadBlobs(upload_requests);
}

digest_string_map
Client::missingDigests(const digest_string_map &directory_map)
{
    std::vector<Digest> digests_in_directory;
    digests_in_directory.reserve(directory_map.size());
    for (const auto &entry : directory_map) {
        digests_in_directory.push_back(entry.first);
    }

    const std::vector<Digest> missing_blobs =
        findMissingBlobs(digests_in_directory);

    digest_string_map missing_files;
    for (const Digest &digest : missing_blobs) {
        missing_files[digest] = directory_map.at(digest);
    }
    return missing_files;
}

} // namespace buildboxcommon
