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
#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_mergeutil.h>

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <grpc/grpc.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>

namespace buildboxcommon {

const size_t Client::s_bytestreamChunkSizeBytes = 1024 * 1024;
// The default limit for gRPC messages is 4 MiB.
// Limit payload to 1 MiB to leave sufficient headroom for metadata.

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
    std::shared_ptr<LocalContentAddressableStorage::StubInterface>
        localCasClient =
            LocalContentAddressableStorage::NewStub(this->d_channel);
    init(bytestreamClient, casClient, localCasClient, capabilitiesClient);
}

void Client::init(
    std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
    std::shared_ptr<ContentAddressableStorage::StubInterface> casClient,
    std::shared_ptr<LocalContentAddressableStorage::StubInterface>
        localCasClient,
    std::shared_ptr<Capabilities::StubInterface> capabilitiesClient)
{
    this->d_bytestreamClient = bytestreamClient;
    this->d_casClient = casClient;
    this->d_localCasClient = localCasClient;
    this->d_capabilitiesClient = capabilitiesClient;

    // Somewhat arbitrary value used as an estimate for
    // the space consumed by gRPC class metadata
    static const size_t MAX_ROOM_FOR_METADATA = 1 << 16;
    // We use this GRPC constant which is a server-side receive value
    // or a client side send value
    // https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h
    this->d_maxBatchTotalSizeBytes =
        GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH - MAX_ROOM_FOR_METADATA;

    BUILDBOX_LOG_INFO("Setting d_maxBatchTotalSizeBytes = "
                      << d_maxBatchTotalSizeBytes << " bytes by default");

    // Request server capabiliies and adjust our defaults according to the
    // server response
    auto getCapabilitiesLambda = [&](grpc::ClientContext &context) {
        GetCapabilitiesRequest request;
        request.set_instance_name(d_instanceName);

        ServerCapabilities response;
        auto status = this->d_capabilitiesClient->GetCapabilities(
            &context, request, &response);
        if (status.ok()) {
            const size_t serverMaxBatchTotalSizeBytes =
                response.cache_capabilities().max_batch_total_size_bytes();
            // 0 means no server limit
            if (serverMaxBatchTotalSizeBytes > 0 &&
                serverMaxBatchTotalSizeBytes <
                    this->d_maxBatchTotalSizeBytes) {
                BUILDBOX_LOG_INFO(
                    "Reconfiguring d_maxBatchTotalSizeBytes down from "
                    << d_maxBatchTotalSizeBytes << " to "
                    << serverMaxBatchTotalSizeBytes
                    << " due to server max_batch_total_size_bytes of "
                    << serverMaxBatchTotalSizeBytes);
                this->d_maxBatchTotalSizeBytes = serverMaxBatchTotalSizeBytes;
            }
        }
        return status;
    };

    try {
        GrpcRetry::retry(getCapabilitiesLambda, this->d_grpcRetryLimit,
                         this->d_grpcRetryDelay,
                         this->d_metadata_attach_function);
    }
    catch (const GrpcError &e) {
        if (e.status.error_code() == grpc::StatusCode::UNIMPLEMENTED) {
            BUILDBOX_LOG_DEBUG(
                "Get capabilities request failed. Using default. "
                << e.what());
        }
        else {
            throw;
        }
    }

    // Generate UUID to use for uploads
    uuid_t uu;
    uuid_generate(uu);
    this->d_uuid = std::string(36, 0);
    uuid_unparse_lower(uu, &this->d_uuid[0]);
}

std::string Client::instanceName() const { return d_instanceName; }

void Client::setInstanceName(const std::string &instance_name)
{
    d_instanceName = instance_name;
}

size_t Client::bytestreamChunkSizeBytes()
{
    return s_bytestreamChunkSizeBytes;
}

void Client::set_tool_details(const std::string &tool_name,
                              const std::string &tool_version)
{
    d_metadata_generator.set_tool_details(tool_name, tool_version);
}

void Client::set_request_metadata(const std::string &action_id,
                                  const std::string &tool_invocation_id,
                                  const std::string &correlated_invocations_id)
{
    d_metadata_generator.set_action_id(action_id);
    d_metadata_generator.set_tool_invocation_id(tool_invocation_id);
    d_metadata_generator.set_correlated_invocations_id(
        correlated_invocations_id);
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
    BUILDBOX_LOG_TRACE("Downloading " << digest.hash() << " to string");
    const std::string resourceName = this->makeResourceName(digest, false);

    std::string result;

    auto fetchLambda = [&](grpc::ClientContext &context) {
        ReadRequest request;
        request.set_resource_name(resourceName);
        request.set_read_offset(0);

        auto reader = this->d_bytestreamClient->Read(&context, request);

        std::string downloaded_data;
        downloaded_data.reserve(digest.size_bytes());

        ReadResponse response;
        while (reader->Read(&response)) {
            downloaded_data += response.data();
        }

        const grpc::Status read_status = reader->Finish();
        if (!read_status.ok()) {
            if (read_status.error_code() == grpc::StatusCode::NOT_FOUND) {
                // If the blob is not found, we don't want `grpcRetry` to
                // re-issue this call. Also, the error code should reach the
                // caller, so we throw it.
                throw GrpcError("Blob not found: " +
                                    read_status.error_message(),
                                read_status);
            }

            return read_status;
        }

        const auto bytes_downloaded =
            static_cast<google::protobuf::int64>(downloaded_data.size());
        if (bytes_downloaded != digest.size_bytes()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Expected "
                                        << digest.size_bytes()
                                        << " bytes, but downloaded blob was "
                                        << bytes_downloaded << " bytes");
        }

        BUILDBOX_LOG_TRACE(resourceName << ": " << bytes_downloaded
                                        << " bytes retrieved");
        result = std::move(downloaded_data);
        return read_status;
    };

    GrpcRetry::retry(fetchLambda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);
    return result;
}

void Client::download(int fd, const Digest &digest)
{
    BUILDBOX_LOG_TRACE("Downloading " << digest.hash() << " to file");
    const std::string resourceName = this->makeResourceName(digest, false);

    auto downloadLambda = [&](grpc::ClientContext &context) {
        ReadRequest request;
        request.set_resource_name(resourceName);
        request.set_read_offset(0);

        auto reader = this->d_bytestreamClient->Read(&context, request);

        ReadResponse response;
        while (reader->Read(&response)) {
            if (write(fd, response.data().c_str(), response.data().size()) <
                0) {
                BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                    std::system_error, errno, std::generic_category,
                    "Error in write to descriptor " << fd);
            }
        }

        const auto read_status = reader->Finish();
        if (!read_status.ok()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                           "Error downloading blob: " +
                                               read_status.error_message());
        }

        struct stat st;
        fstat(fd, &st);
        if (st.st_size != digest.size_bytes()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Expected "
                                        << digest.size_bytes()
                                        << " bytes, but downloaded blob was "
                                        << st.st_size << " bytes");
        }
        BUILDBOX_LOG_TRACE(resourceName << ": " << st.st_size
                                        << " bytes retrieved");
        return read_status;
    };

    GrpcRetry::retry(downloadLambda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);
}

void Client::downloadDirectory(
    const Digest &digest, const std::string &path,
    const download_callback_t &download_callback,
    const return_directory_callback_t &return_directory_callback)
{
    const Directory directory = return_directory_callback(digest);

    // Downloading the files in this directory:
    OutputMap outputs;
    std::vector<Digest> file_digests;
    file_digests.reserve(static_cast<size_t>(directory.files_size()));

    for (const FileNode &file : directory.files()) {
        file_digests.push_back(file.digest());

        const std::string file_path = path + "/" + file.name();
        outputs.emplace(
            file.digest().hash(),
            std::pair<std::string, bool>(file_path, file.is_executable()));
    }
    download_callback(file_digests, outputs);

    // Creating the subdirectories in this level and recursively fetching
    // their contents:
    for (const DirectoryNode &directory_node : directory.directories()) {
        const std::string directory_path = path + "/" + directory_node.name();
        if (mkdir(directory_path.c_str(), 0777) == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error in mkdir for directory \"" << directory_path << "\"");
        }

        downloadDirectory(directory_node.digest(), directory_path,
                          download_callback, return_directory_callback);
    }

    // Create symlinks, note: we just create the symlink. It's not the
    // responsibility of the worker/casd, to ensure the target is valid and
    // has contents.
    for (const SymlinkNode &symlink_node : directory.symlinks()) {
        if (symlink_node.target().empty() || symlink_node.name().empty()) {
            BUILDBOX_LOG_WARNING(
                "Symlink Node name or target empty skipping.");
            continue;
        }
        // Prepend the path to the symlink_node name.
        const std::string symlink_path = path + "/" + symlink_node.name();

        if (symlink(symlink_node.target().c_str(), symlink_path.c_str()) !=
            0) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Unable to create symlink: \""
                    << symlink_path + "\" to target: \""
                    << symlink_node.target() << "\"");
        }
    }
}

void Client::downloadDirectory(const Digest &digest, const std::string &path)
{
    download_callback_t download_blobs =
        [this](const std::vector<Digest> &file_digests,
               const OutputMap &outputs) {
            this->downloadBlobs(file_digests, outputs);
        };

    return_directory_callback_t download_directory =
        [this](const Digest &message_digest) {
            return this->fetchMessage<Directory>(message_digest);
        };

    this->downloadDirectory(digest, path, download_blobs, download_directory);
}

void Client::upload(const std::string &data, const Digest &digest)
{
    BUILDBOX_LOG_DEBUG("Uploading " << digest.hash() << " from string");
    const auto data_size = static_cast<google::protobuf::int64>(data.size());

    if (data_size != digest.size_bytes()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::logic_error, "Digest length of "
                                  << digest.size_bytes() << " bytes for "
                                  << digest.hash()
                                  << " does not match string length of "
                                  << data_size << " bytes");
    }

    const std::string resourceName = this->makeResourceName(digest, true);

    auto uploadLambda = [&](grpc::ClientContext &context) {
        WriteResponse response;
        auto writer = this->d_bytestreamClient->Write(&context, &response);

        size_t offset = 0;
        bool lastChunk = false;
        while (!lastChunk) {
            WriteRequest request;
            request.set_resource_name(resourceName);
            request.set_write_offset(
                static_cast<google::protobuf::int64>(offset));

            const size_t uploadLength =
                std::min(bytestreamChunkSizeBytes(), data.size() - offset);

            request.set_data(&data[offset], uploadLength);
            offset += uploadLength;

            lastChunk = (offset == data.size());
            if (lastChunk) {
                request.set_finish_write(lastChunk);
            }

            if (!writer->Write(request)) {
                BUILDBOXCOMMON_THROW_EXCEPTION(
                    std::runtime_error,
                    "Upload of " << digest.hash() << " failed: broken stream");
            }
        }

        writer->WritesDone();
        auto status = writer->Finish();
        if (static_cast<google::protobuf::int64>(offset) !=
            digest.size_bytes()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error,
                "Expected to upload "
                    << digest.size_bytes() << " bytes for " << digest.hash()
                    << ", but uploaded blob was " << offset << " bytes");
        }

        BUILDBOX_LOG_DEBUG(resourceName << ": " << offset
                                        << " bytes uploaded");
        return status;
    };

    GrpcRetry::retry(uploadLambda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);
}

void Client::upload(int fd, const Digest &digest)
{
    std::vector<char> buffer(bytestreamChunkSizeBytes());
    BUILDBOX_LOG_DEBUG("Uploading " << digest.hash() << " from file");

    const std::string resourceName = this->makeResourceName(digest, true);

    lseek(fd, 0, SEEK_SET);

    auto uploadLambda = [&](grpc::ClientContext &context) {
        WriteResponse response;
        auto writer = this->d_bytestreamClient->Write(&context, &response);

        ssize_t offset = 0;
        bool lastChunk = false;
        while (!lastChunk) {
            const ssize_t bytesRead =
                read(fd, &buffer[0], bytestreamChunkSizeBytes());
            if (bytesRead < 0) {
                BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                    std::system_error, errno, std::generic_category,
                    "Error in read on descriptor " << fd);
            }

            WriteRequest request;
            request.set_resource_name(resourceName);
            request.set_write_offset(offset);
            request.set_data(&buffer[0], static_cast<size_t>(bytesRead));

            if (offset + bytesRead < digest.size_bytes()) {
                if (bytesRead == 0) {
                    BUILDBOXCOMMON_THROW_EXCEPTION(
                        std::runtime_error,
                        "Upload of " << digest.hash()
                                     << " failed: unexpected end of file");
                }
            }
            else {
                lastChunk = true;
                request.set_finish_write(true);
            }

            if (!writer->Write(request)) {
                BUILDBOXCOMMON_THROW_EXCEPTION(
                    std::runtime_error,
                    "Upload of " << digest.hash() << " failed: broken stream");
            }

            offset += bytesRead;
        }

        writer->WritesDone();
        auto status = writer->Finish();
        if (offset != digest.size_bytes()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                           "Upload of " << digest.hash()
                                                        << " failed");
        }

        BUILDBOX_LOG_DEBUG(resourceName << ": " << offset
                                        << " bytes uploaded");
        return status;
    };

    GrpcRetry::retry(uploadLambda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);
}

void Client::uploadRequest(const UploadRequest &request)
{
    if (request.path.empty()) {
        upload(request.data, request.digest);
    }
    else {
        const int fd = open(request.path.c_str(), O_RDONLY);
        if (fd < 0) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error in open for file \"" << request.path << "\"");
        }
        try {
            upload(fd, request.digest);
        }
        catch (...) {
            close(fd);
            throw;
        }
        close(fd);
    }
}

std::vector<Client::UploadResult>
Client::uploadBlobs(const std::vector<UploadRequest> &requests)
{
    return Client::uploadBlobs(requests, false);
}

std::vector<Client::UploadResult>
Client::uploadBlobs(const std::vector<UploadRequest> &requests,
                    const bool throw_on_error)
{
    std::vector<Client::UploadResult> results;

    // We first sort the requests by their sizes in ascending order, so
    // that we can then iterate through that result greedily trying to add
    // as many digests as possible to each request.
    std::vector<UploadRequest> request_list(requests);
    std::sort(request_list.begin(), request_list.end(),
              [](const UploadRequest &r1, const UploadRequest &r2) {
                  return r1.digest.size_bytes() < r2.digest.size_bytes();
              });

    // Grouping the requests into batches (we only need to look at the
    // Digests for their sizes):
    std::vector<Digest> digests;
    for (const auto &r : request_list) {
        digests.push_back(r.digest);
    }

    const auto batches = makeBatches(digests);
    for (const auto &batch_range : batches) {
        const size_t batch_start = batch_range.first;
        const size_t batch_end = batch_range.second;

        try {
            const std::vector<Client::UploadResult> digests_not_uploaded =
                batchUpload(request_list, batch_start, batch_end);
            std::move(digests_not_uploaded.cbegin(),
                      digests_not_uploaded.cend(),
                      std::back_inserter(results));
        }
        catch (const std::runtime_error &e) {
            BUILDBOX_LOG_ERROR("Batch upload failed: " +
                               std::string(e.what()));

            // The whole batch request failed.
            if (throw_on_error) {
                throw e;
            }

            // Report an INTERNAL error code for failed uploads in case
            // there is no throw.
            const auto failed_status = grpc::Status(grpc::StatusCode::INTERNAL,
                                                    grpc::string(e.what()));
            for (auto d = batch_start; d < batch_end; d++) {
                results.emplace_back(request_list.at(d).digest, failed_status);
            }
        }
    }

    // Fetching all those digests that might need to be uploaded using the
    // Bytestream API. Those will be in the range [batch_end,
    // batches.size()).
    const size_t batch_end = batches.empty() ? 0 : batches.rbegin()->second;

    for (auto d = batch_end; d < request_list.size(); d++) {
        try {
            uploadRequest(request_list[d]);
        }
        catch (const GrpcError &e) {
            if (throw_on_error) {
                BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                               "Failed to upload blob: " +
                                                   e.status.error_message());
            }
            results.emplace_back(request_list[d].digest, e.status);
        }
        catch (const std::runtime_error &e) {
            BUILDBOX_LOG_ERROR("Failed to upload blob: " +
                               std::string(e.what()));
            if (throw_on_error) {
                throw e;
            }
            results.emplace_back(
                request_list[d].digest,
                grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
        }
    }

    return results;
}

Client::DownloadBlobsResult
Client::downloadBlobs(const std::vector<Digest> &digests)
{
    Client::DownloadBlobsResult downloaded_data;

    // Writing the data directly into the result. (We know that the status code
    // will be OK for each of these blobs.)
    auto write_blob = [&](const std::string &hash, const std::string &data) {
        google::rpc::Status status;
        status.set_code(grpc::StatusCode::OK);

        downloaded_data.emplace(hash, std::make_pair(status, data));
    };

    const Client::DownloadResults download_results =
        downloadBlobs(digests, write_blob, false);

    // And adding the codes of the hashes that failed into the result:
    for (const auto &entry : download_results) {
        const Digest &digest = entry.first;
        const google::rpc::Status &status = entry.second;

        if (status.code() != grpc::StatusCode::OK) {
            downloaded_data.emplace(digest.hash(), std::make_pair(status, ""));
        }
    }

    return downloaded_data;
}

void Client::downloadBlobs(const std::vector<Digest> &digests,
                           const OutputMap &outputs)
{

    auto write_blob = [&](const std::string &hash, const std::string &data) {
        const std::pair<OutputMap::const_iterator, OutputMap::const_iterator>
            range = outputs.equal_range(hash);

        for (auto it = range.first; it != range.second; it++) {
            const std::string path = it->second.first;
            const bool is_executable = it->second.second;

            mode_t file_permissions =
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
            if (is_executable) {
                file_permissions |= S_IXUSR | S_IXGRP | S_IXOTH; // 0755
            }

            const int write_status =
                FileUtils::writeFileAtomically(path, data, file_permissions);
            if (write_status != 0 && write_status != EEXIST) {
                // `EEXIST` means someone beat us to writing the file, which is
                // not an error assuming the contents are the same.
                BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                    std::system_error, write_status, std::system_category,
                    "Could not atomically write blob with digest \""
                        << hash << "/" << data.size() << "\" to \"" << path
                        << "\"");
            }
        }
    };

    downloadBlobs(digests, write_blob, true);
    // ^ If an error is encountered during download, aborts by throwing an
    // exception.
}

Client::DownloadResults
Client::downloadBlobs(const std::vector<Digest> &digests,
                      const WriteBlobCallback &write_blob, bool throw_on_error)
{
    DownloadResults download_results;
    download_results.reserve(digests.size());

    // We first sort the digests by their sizes in ascending order, so that
    // we can then iterate through that result greedily trying to add as
    // many digests as possible to each request.
    auto request_list(digests);
    std::sort(request_list.begin(), request_list.end(),
              [](const Digest &d1, const Digest &d2) {
                  return d1.size_bytes() < d2.size_bytes();
              });

    const auto batches = makeBatches(request_list);
    for (const auto &batch_range : batches) {
        const size_t batch_start = batch_range.first;
        const size_t batch_end = batch_range.second;

        // For each batch, we make the request and then store whether it was
        // successful in the result:
        try {
            const auto batch_results = batchDownload(request_list, batch_start,
                                                     batch_end, write_blob);
            std::move(batch_results.cbegin(), batch_results.cend(),
                      std::back_inserter(download_results));
        }
        catch (const std::runtime_error &e) {
            BUILDBOX_LOG_ERROR("Batch download failed: " +
                               std::string(e.what()));

            // The whole batch request failed.
            if (throw_on_error) {
                throw e;
            }

            // If we don't throw, we need to report in the result that the
            // requested digests failed:
            google::rpc::Status failed_status;
            failed_status.set_code(grpc::StatusCode::INTERNAL);

            for (auto d = batch_start; d < batch_end; d++) {
                const Digest digest = request_list.at(d);
                download_results.emplace_back(digest, failed_status);
            }
        }
    }

    // Fetching all those digests that might need to be downloaded using
    // the Bytestream API. Those will be in the range [batch_end,
    // batches.size()).
    size_t batch_end;
    if (batches.empty()) {
        batch_end = 0;
    }
    else {
        batch_end = batches.rbegin()->second;
    }

    for (auto d = batch_end; d < request_list.size(); d++) {
        const Digest digest = request_list[d];

        google::rpc::Status download_status;
        try {
            const auto data = fetchString(digest);
            write_blob(digest.hash(), data);
            download_status.set_code(grpc::StatusCode::OK);
        }
        catch (const GrpcError &e) {
            if (throw_on_error) {
                BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                               "Failed to download string: " +
                                                   e.status.error_message());
            }

            download_status.set_code(e.status.error_code());
            download_status.set_message(e.status.error_message());
        }
        catch (const std::runtime_error &e) {
            BUILDBOX_LOG_ERROR("Error: fetchString(): " +
                               std::string(e.what()));
            if (throw_on_error) {
                throw e;
            }

            download_status.set_code(grpc::StatusCode::INTERNAL);
        }

        download_results.emplace_back(digest, download_status);
    }

    return download_results;
}

std::unique_ptr<Client::StagedDirectory>
Client::stage(const Digest &root_digest, const std::string &path) const
{
    auto context = std::make_shared<grpc::ClientContext>();

    std::shared_ptr<
        grpc::ClientReaderWriterInterface<StageTreeRequest, StageTreeResponse>>
        reader_writer(d_localCasClient->StageTree(context.get()));

    StageTreeRequest request;
    request.set_instance_name(d_instanceName);
    request.mutable_root_digest()->CopyFrom(root_digest);
    request.set_path(path);

    const bool succesful_write = reader_writer->Write(request);
    if (!succesful_write) {
        const grpc::Status write_error_status = reader_writer->Finish();
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Error staging \"" << toString(root_digest) << "\" into \"" << path
                               << "\": \""
                               << write_error_status.error_message() << "\"");
    }

    StageTreeResponse response;
    const bool succesful_read = reader_writer->Read(&response);
    if (!succesful_read) {
        reader_writer->WritesDone();
        const grpc::Status read_error_status = reader_writer->Finish();
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Error staging \"" << toString(root_digest) << "\" into \"" << path
                               << "\": \"" << read_error_status.error_message()
                               << "\"");
    }

    return std::make_unique<StagedDirectory>(context, reader_writer,
                                             response.path());
}

std::vector<Directory> Client::getTree(const Digest &root_digest)
{
    grpc::ClientContext context;
    GetTreeRequest request;
    request.set_instance_name(d_instanceName);
    request.mutable_root_digest()->CopyFrom(root_digest);

    std::unique_ptr<grpc::ClientReaderInterface<GetTreeResponse>> reader(
        d_casClient->GetTree(&context, request));

    std::vector<Directory> tree;
    GetTreeResponse response;
    while (reader->Read(&response)) {
        BUILDBOX_LOG_TRACE("\n" << response.directories());
        tree.insert(tree.end(), response.directories().begin(),
                    response.directories().end());
    }

    grpc::Status status = reader->Finish();
    if (!status.ok()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Error getting tree for digest \""
                                           << toString(root_digest)
                                           << "\", status = ["
                                           << status.error_code() << ": \""
                                           << status.error_message() + "\"]");
    }

    return tree;
}

CaptureTreeResponse
Client::captureTree(const std::vector<std::string> &paths,
                    const std::vector<std::string> &properties,
                    bool bypass_local_cache) const
{
    CaptureTreeRequest request;
    request.set_instance_name(d_instanceName);
    request.set_bypass_local_cache(bypass_local_cache);

    for (const std::string &path : paths) {
        request.add_path(path);
    }

    for (const std::string &property : properties) {
        request.add_node_properties(property);
    }

    CaptureTreeResponse response;

    const auto captureLambda = [&](grpc::ClientContext &context) {
        return d_localCasClient->CaptureTree(&context, request, &response);
    };

    GrpcRetry::retry(captureLambda, d_grpcRetryLimit, d_grpcRetryDelay);
    return response;
}

CaptureFilesResponse
Client::captureFiles(const std::vector<std::string> &paths,
                     const std::vector<std::string> &properties,
                     bool bypass_local_cache) const
{
    CaptureFilesRequest request;
    request.set_instance_name(d_instanceName);
    request.set_bypass_local_cache(bypass_local_cache);

    for (const std::string &path : paths) {
        request.add_path(path);
    }

    for (const std::string &property : properties) {
        request.add_node_properties(property);
    }

    CaptureFilesResponse response;

    const auto captureLambda = [&](grpc::ClientContext &context) {
        return d_localCasClient->CaptureFiles(&context, request, &response);
    };

    GrpcRetry::retry(captureLambda, d_grpcRetryLimit, d_grpcRetryDelay);
    return response;
}

std::vector<Client::UploadResult>
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
        entry->set_data(
            requests[d].path.empty()
                ? requests[d].data
                : FileUtils::getFileContents(requests[d].path.c_str()));
    }

    BUILDBOX_LOG_TRACE("BatchUpdateBlobs Request serialized message size = "
                       << request.ByteSizeLong());

    BatchUpdateBlobsResponse response;
    auto batchUploadLamda = [&](grpc::ClientContext &context) {
        const auto status =
            this->d_casClient->BatchUpdateBlobs(&context, request, &response);
        return status;
    };

    GrpcRetry::retry(batchUploadLamda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);

    BUILDBOX_LOG_TRACE("BatchUpdateBlobs Response serialized message size = "
                       << response.ByteSizeLong());

    std::vector<Client::UploadResult> results;
    for (const auto &uploadResponse : response.responses()) {
        if (uploadResponse.status().code() != GRPC_STATUS_OK) {
            results.emplace_back(
                uploadResponse.digest(),
                grpc::Status(grpc::StatusCode(uploadResponse.status().code()),
                             uploadResponse.status().message()));
        }
    }
    return results;
}

Client::DownloadResults
Client::batchDownload(const std::vector<Digest> &digests,
                      const size_t start_index, const size_t end_index,
                      const WriteBlobCallback &write_blob_function)
{
    assert(start_index <= end_index);
    assert(end_index <= digests.size());

    BatchReadBlobsRequest request;
    request.set_instance_name(d_instanceName);

    for (auto d = start_index; d < end_index; d++) {
        auto digest = request.add_digests();
        digest->CopyFrom(digests[d]);
    }
    BUILDBOX_LOG_TRACE("BatchReadBlobs Request serialized message size = "
                       << request.ByteSizeLong());

    BatchReadBlobsResponse response;
    auto batchDownloadLamda = [&](grpc::ClientContext &context) {
        const auto status =
            this->d_casClient->BatchReadBlobs(&context, request, &response);
        return status;
    };

    GrpcRetry::retry(batchDownloadLamda, this->d_grpcRetryLimit,
                     this->d_grpcRetryDelay, this->d_metadata_attach_function);

    BUILDBOX_LOG_TRACE("BatchReadBlobs Response serialized message size = "
                       << response.ByteSizeLong());

    DownloadResults download_results;
    download_results.reserve(static_cast<size_t>(response.responses_size()));

    for (const auto &downloadResponse : response.responses()) {
        if (downloadResponse.status().code() == GRPC_STATUS_OK) {
            write_blob_function(downloadResponse.digest().hash(),
                                downloadResponse.data());
        }

        download_results.emplace_back(downloadResponse.digest(),
                                      downloadResponse.status());
    }

    return download_results;
}

std::vector<std::pair<size_t, size_t>>
Client::makeBatches(const std::vector<Digest> &digests)
{
    // The values below are set based on rounding up sizeof's the gRPC
    // classes BatchUpdateBlobsRequest(upload) and
    // BatchReadBlobsRequest(download). This is an attempt to factor in these
    // values into the overall batch size equation
    static const size_t SIZEOF_ESTIMATED_TOP_LEVEL_GRPC_CONTAINER = 256;
    static const size_t SIZEOF_ESTIMATED_NESTED_GRPC_CONTAINERS = 50;

    // A batch is a pair of indexes into the vector of `digests` that
    // can all fit in one `d_maxBatchTotalSizeBytes` sized buffer;
    // The indices are semantically represented by [batch_start, batch_end)
    std::vector<std::pair<size_t, size_t>> batches;
    const size_t max_batch_size =
        d_maxBatchTotalSizeBytes - SIZEOF_ESTIMATED_TOP_LEVEL_GRPC_CONTAINER -
        (SIZEOF_ESTIMATED_NESTED_GRPC_CONTAINERS * digests.size());
    size_t batch_start = 0;
    size_t batch_end = 0;
    while (batch_end < digests.size()) {
        size_t bytes_in_batch = 0;
        if (static_cast<size_t>(digests[batch_end].size_bytes()) >
            max_batch_size) {
            // All digests from `batch_end` to the end of the list are
            // larger than what we can request; stop.
            return batches;
        }

        // Adding all the digests that we can until we run out or exceed
        // the batch request limit...
        while (batch_end < digests.size() &&
               bytes_in_batch + digests[batch_end].size_bytes() <=
                   max_batch_size) {
            bytes_in_batch += digests[batch_end].size_bytes();
            batch_end++;
        }

        batches.emplace_back(std::make_pair(batch_start, batch_end));
        batch_start = batch_end;
    }

    return batches;
}

std::vector<Digest>
Client::findMissingBlobs(const std::vector<Digest> &digests)
{
    FindMissingBlobsRequest request;
    request.set_instance_name(d_instanceName);

    // We take the given digests and split them across requests to not exceed
    // the maximum size of a gRPC message:
    std::vector<FindMissingBlobsRequest> requests_to_issue;
    size_t batch_size = 0;
    for (const Digest &digest : digests) {
        const size_t digest_size = digest.ByteSizeLong();
        if (batch_size + digest_size > bytestreamChunkSizeBytes()) {
            requests_to_issue.push_back(request);
            request.clear_blob_digests();
            batch_size = 0;
        }
        else {
            batch_size += digest_size;
        }

        auto entry = request.add_blob_digests();
        entry->CopyFrom(digest);
    }
    requests_to_issue.push_back(request);

    std::vector<Digest> missing_blobs;
    for (const auto &request_to_issue : requests_to_issue) {
        FindMissingBlobsResponse response;
        auto findMissingBlobsLambda = [&](grpc::ClientContext &context) {
            const auto status = this->d_casClient->FindMissingBlobs(
                &context, request_to_issue, &response);
            return status;
        };

        GrpcRetry::retry(findMissingBlobsLambda, this->d_grpcRetryLimit,
                         this->d_grpcRetryDelay,
                         this->d_metadata_attach_function);

        missing_blobs.insert(missing_blobs.end(),
                             response.missing_blob_digests().cbegin(),
                             response.missing_blob_digests().cend());
    }

    return missing_blobs;
}

std::vector<Client::UploadResult>
Client::uploadDirectory(const std::string &path, Digest *root_directory_digest,
                        Tree *tree)
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
                FileUtils::getFileContents(entry.second.c_str());
            upload_requests.emplace_back(UploadRequest(digest, file_contents));
        }
    }

    if (tree != nullptr) {
        tree->CopyFrom(nested_dir.to_tree());
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

Client::StagedDirectory::StagedDirectory(
    std::shared_ptr<grpc::ClientContext> context,
    std::shared_ptr<
        grpc::ClientReaderWriterInterface<StageTreeRequest, StageTreeResponse>>
        reader_writer,
    const std::string &path)
    : d_context(context), d_reader_writer(reader_writer), d_path(path)
{
}

Client::StagedDirectory::~StagedDirectory()
{
    // According to the LocalCAS spec., an empty request tells the
    // server to clean up.
    d_reader_writer->Write(StageTreeRequest());
    d_reader_writer->WritesDone();
}

} // namespace buildboxcommon
