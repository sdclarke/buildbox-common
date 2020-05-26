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

#ifndef INCLUDED_BUILDBOXCOMMON_CLIENT
#define INCLUDED_BUILDBOXCOMMON_CLIENT

#include <functional>
#include <unordered_map>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_merklize.h>
#include <buildboxcommon_protos.h>
#include <buildboxcommon_requestmetadata.h>

namespace buildboxcommon {

/**
 * Implements a mechanism to communicate with remote CAS servers, and includes
 * data members to keep track of an ongoing batch upload or batch download
 * request.
 */
class Client {
  public:
    Client(){};

    Client(std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
           std::shared_ptr<ContentAddressableStorage::StubInterface> casClient,
           std::shared_ptr<LocalContentAddressableStorage::StubInterface>
               localCasClient,
           std::shared_ptr<Capabilities::StubInterface> capabilitiesClient,
           int64_t maxBatchTotalSizeBytes =
               static_cast<int64_t>(s_bytestreamChunkSizeBytes))
        : d_bytestreamClient(bytestreamClient), d_casClient(casClient),
          d_localCasClient(localCasClient),
          d_capabilitiesClient(capabilitiesClient),
          d_maxBatchTotalSizeBytes(maxBatchTotalSizeBytes)
    {
    }
    /**
     * Connect to the CAS server with the given connection options.
     */
    void init(const ConnectionOptions &options);

    /**
     * Connect to the CAS server with the given clients.
     */
    void
    init(std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
         std::shared_ptr<ContentAddressableStorage::StubInterface> casClient,
         std::shared_ptr<LocalContentAddressableStorage::StubInterface>
             d_localCasClient,
         std::shared_ptr<Capabilities::StubInterface> capabilitiesClient);

    void set_tool_details(const std::string &tool_name,
                          const std::string &tool_version);
    /**
     * Set the optional ID values to be attached to requests.
     */
    void set_request_metadata(const std::string &action_id,
                              const std::string &tool_invocation_id,
                              const std::string &correlated_invocations_id);

    /**
     * Download the blob with the given digest and return it.
     *
     * If the server returned an error, or the size of the received blob does
     * not match the digest, throw an `std::runtime_error` exception.
     */
    std::string fetchString(const Digest &digest);

    /**
     * Download the blob with the given digest to the given file descriptor.
     *
     * If the file descriptor cannot be written to, the size of the
     * received blob does not match the digest, or the server
     * returned an error, throw an `std::runtime_error` exception.
     */
    void download(int fd, const Digest &digest);

    void downloadDirectory(const Digest &digest, const std::string &path);

    /**
     * Upload the given string. If it can't be uploaded successfully, throw
     * an exception.
     */
    void upload(const std::string &data, const Digest &digest);

    /**
     * Upload a blob from the given file descriptor. If it can't be uploaded
     * successfully, throw an exception.
     */
    void upload(int fd, const Digest &digest);

    struct UploadRequest {
        Digest digest;
        std::string data;
        std::string path;

        UploadRequest(const Digest &_digest, const std::string _data)
            : digest(_digest), data(_data){};

        static UploadRequest from_path(const Digest &_digest,
                                       const std::string _path)
        {
            auto request = UploadRequest(_digest);
            request.path = _path;
            return request;
        }

      private:
        UploadRequest(const Digest &_digest) : digest(_digest){};
    };

    struct UploadResult {
        Digest digest;
        grpc::Status status;

        UploadResult(const Digest &_digest, const grpc::Status &_status)
            : digest(_digest), status(_status){};
    };

    /* Upload multiple digests in an efficient way, allowing each digest to
     * potentially fail separately.
     *
     * Return a list containing the Digests that failed to be uploaded and the
     * errors they received. (An empty result indicates that all digests were
     * uploaded.)
     */
    std::vector<UploadResult>
    uploadBlobs(const std::vector<UploadRequest> &requests);

    typedef std::unordered_map<std::string,
                               std::pair<google::rpc::Status, std::string>>
        DownloadBlobsResult;

    /* Given a list of digests, download the data and return it in a map
     * indexed by hash. Allow each digest to potentially fail separately.
     *
     * The returned map's values are pairs of (status, data) where the second
     * component will be empty if the status contains an non-OK code.
     */
    DownloadBlobsResult downloadBlobs(const std::vector<Digest> &digests);

    /* Given a list of digests, download the data and store each blob in the
     * path specified by the entry's first member in the `outputs` map. If the
     * second member of the tuple is true, mark the file as executable.
     *
     * If any errors are encountered in the process of fetching the blobs, it
     * aborts and throws an `std::runtime_error` exception. (It might leave
     * directories in an inconsistent state, i.e. with missing files.)
     */
    typedef std::unordered_multimap<std::string, std::pair<std::string, bool>>
        OutputMap;
    void downloadBlobs(const std::vector<Digest> &digests,
                       const OutputMap &outputs);

    /**
     * Given a list of digests, creates and sends a `FindMissingBlobsRequest`
     * to the server.
     *
     * Returns a list of Digests that the remote server reports not having,
     * or throws a runtime_exception if the request failed.
     */
    std::vector<Digest> findMissingBlobs(const std::vector<Digest> &digests);

    /**
     * Uploads the contents of the given path.
     *
     * Returns a list of Digests that the remote server reports not having,
     * or throws a runtime_exception if the request failed.
     *
     * If the optional `directory_digest` pointer is provided, the Digest of
     * the uploaded directory is copied to it. Likewise for the `tree`
     * pointer and the `Tree` object that is generated for the directory.
     */
    std::vector<UploadResult>
    uploadDirectory(const std::string &path,
                    Digest *directory_digest = nullptr, Tree *tree = nullptr);

    /*
     * Send a LocalCas protocol `CaptureTree()` request containing the given
     * paths and the given property names. If successful, returns a
     * `CaptureTreeResponse` object (it contains a Status for each path).
     *
     * If the request fails, throws an `std::runtime_exception`.
     */
    CaptureTreeResponse captureTree(const std::vector<std::string> &paths,
                                    const std::vector<std::string> &properties,
                                    bool bypass_local_cache) const;

    /*
     * Send a LocalCas protocol `CaptureFiles()` request containing the given
     * paths and the given property names. If successful, returns a
     * `CaptureFilesResponse` object (it contains a Status for each path).
     *
     * If the request fails, throws an `std::runtime_exception`.
     */
    CaptureFilesResponse
    captureFiles(const std::vector<std::string> &paths,
                 const std::vector<std::string> &properties,
                 bool bypass_local_cache) const;

    class StagedDirectory {
        /*
         * Represents a staged directory. It encapsulates the gRPC stream's
         * status, keeping it open (and preventing the server from cleaning
         * up).
         *
         * On destruction it sends an empty message to the server to clean up
         * the directory.
         */

      public:
        explicit StagedDirectory(
            std::shared_ptr<grpc::ClientContext> context,
            std::shared_ptr<grpc::ClientReaderWriterInterface<
                StageTreeRequest, StageTreeResponse>>
                reader_writer,
            const std::string &path);

        ~StagedDirectory();

        // Do now allow making copies:
        StagedDirectory(const StagedDirectory &) = delete;
        StagedDirectory &operator=(const StagedDirectory &) = delete;

        inline std::string path() const { return d_path; }

      private:
        const std::shared_ptr<grpc::ClientContext> d_context;
        const std::shared_ptr<grpc::ClientReaderWriterInterface<
            StageTreeRequest, StageTreeResponse>>
            d_reader_writer;
        const std::string d_path;
    };

    /**
     * Stage a directory using the LocalCAS `Stage()` call.
     *
     * The `path` parameter is optional. If not provided, the server will
     * assign a temporary directory.
     *
     * On success return a `unique_ptr` to a `StagedDirectory` object that when
     * destructed will request the server to clean up.
     *
     * On error throw an `std::runtime_error` exception.
     */
    std::unique_ptr<StagedDirectory> stage(const Digest &root_digest,
                                           const std::string &path = "") const;

    /**
     * Get the Directory tree whose root digest is 'digest', using the CAS
     * GetTree() call.
     *
     * On success, return a vector of protobuf Directory structures
     *
     * On error throw an 'std::runtime_error' exception
     */
    std::vector<Directory> getTree(const Digest &digest);

    /**
     * Fetch the Protocol Buffer message of the given type and digest and
     * deserialize it.
     */
    template <typename Msg> inline Msg fetchMessage(const Digest &digest)
    {
        Msg result;
        if (!result.ParseFromString(this->fetchString(digest))) {
            throw std::runtime_error("Could not deserialize fetched message");
        }
        return result;
    }

    /**
     * Upload the given Protocol Buffer message to CAS and return its
     * Digest.
     */
    template <typename Msg> inline Digest uploadMessage(const Msg &msg)
    {
        const std::string str = msg.SerializeAsString();
        const Digest digest = CASHash::hash(str);
        this->upload(str, digest);
        return digest;
    }

    std::string instanceName() const;

    void setInstanceName(const std::string &instance_name);

    static size_t bytestreamChunkSizeBytes();

  protected:
    typedef std::function<void(const std::string &hash,
                               const std::string &data)>
        WriteBlobCallback;

    typedef std::pair<Digest, google::rpc::Status> DownloadResult;
    typedef std::vector<DownloadResult> DownloadResults;

    /* Download the digests in the specified list and invoke the
     * `write_blob_callback` function after each blob is downloaded.
     *
     * `throw_on_error` determines whether an `std::runtime_error`
     * exception is to be raised on encountering an error during a
     * download.
     *
     * Note: marked as `protected` to unit-test.
     */
    DownloadResults downloadBlobs(const std::vector<Digest> &digests,
                                  const WriteBlobCallback &write_blob_callback,
                                  bool throw_on_error);

    typedef std::function<void(const std::vector<Digest> &digest,
                               const OutputMap &outputs)>
        download_callback_t;
    typedef std::function<Directory(const Digest &digest)>
        return_directory_callback_t;

    void downloadDirectory(
        const Digest &digest, const std::string &path,
        const download_callback_t &download_callback,
        const return_directory_callback_t &return_directory_callback);

    /* Upload multiple digests in an efficient way, allowing each digest to
     * potentially fail separately.
     *
     * Return a list containing the Digests that failed to be uploaded and the
     * errors they received. (An empty result indicates that all digests were
     * uploaded.)
     *
     * `throw_on_error` determines whether an `std::runtime_error`
     * exception is to be raised on encountering an error during a
     * download.
     *
     * Note: marked as `protected` to unit-test.
     */
    std::vector<UploadResult>
    uploadBlobs(const std::vector<UploadRequest> &requests,
                const bool throw_on_error);

  private:
    std::shared_ptr<grpc::Channel> d_channel;
    std::shared_ptr<ByteStream::StubInterface> d_bytestreamClient;
    std::shared_ptr<ContentAddressableStorage::StubInterface> d_casClient;
    std::shared_ptr<LocalContentAddressableStorage::StubInterface>
        d_localCasClient;
    std::shared_ptr<Capabilities::StubInterface> d_capabilitiesClient;

    // initialized here to prevent errors, in case options are not passed into
    // init
    int d_grpcRetryLimit = 0;
    int d_grpcRetryDelay = 100;

    int64_t d_maxBatchTotalSizeBytes;

    std::string d_uuid;
    std::string d_instanceName;

    RequestMetadataGenerator d_metadata_generator;
    const std::function<void(grpc::ClientContext *)>
        d_metadata_attach_function = [&](grpc::ClientContext *context) {
            d_metadata_generator.attach_request_metadata(context);
        };

    // Maximum number of bytes that can be sent in a single gRPC message.
    static const size_t s_bytestreamChunkSizeBytes;

    std::string makeResourceName(const Digest &digest, bool is_upload);

    /* Uploads the requests contained in the range [start_index,
     * end_index).
     *
     * The sum of bytes of the data in this range MUST NOT exceed the
     * maximum batch size request allowed.
     */
    std::vector<UploadResult>
    batchUpload(const std::vector<UploadRequest> &requests,
                const size_t start_index, const size_t end_index);

    /* Downloads the data for the Digests stored in the range
     * [start_index, end_index) of the given vector.
     *
     * The sum of sizes inside the range MUST NOT exceed the maximum batch
     * size request allowed.
     *
     * The `write_blob_function` callback will be invoked for each blob that is
     * successfully fetched.
     *
     * Returns a map with the status code received for each Digest.
     */
    DownloadResults
    batchDownload(const std::vector<Digest> &digests, const size_t start_index,
                  const size_t end_index,
                  const WriteBlobCallback &write_blob_function);

    /* Given a list of digests sorted by increasing size, forms batches
     * according to the value of `d_maxBatchTotalSizeBytes`.
     */
    std::vector<std::pair<size_t, size_t>>
    makeBatches(const std::vector<Digest> &digests);

    /* Given a directory map, invoke `findMissingBlobs()` and return a map
     * with the subset of protos that need to be uploaded.
     */
    digest_string_map missingDigests(const digest_string_map &directory_map);

    /* Upload a single request. */
    void uploadRequest(const UploadRequest &request);

    /*
     * RequestMetadata values. They will be attached to requests sent by this
     * client.
     */
    ToolDetails d_tool_details;
    std::string d_action_id;
    std::string d_tool_invocation_id;
    std::string d_correlated_invocations_id;
};

} // namespace buildboxcommon

#endif
