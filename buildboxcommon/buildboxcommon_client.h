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

#include <unordered_map>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_protos.h>

namespace buildboxcommon {

#define BYTESTREAM_CHUNK_SIZE (1024 * 1024)

/**
 * Implements a mechanism to communicate with remote CAS servers, and includes
 * data members to keep track of an ongoing batch upload or batch download
 * request.
 */
class Client {
  private:
    std::string makeResourceName(const Digest &digest, bool is_upload);

    std::shared_ptr<grpc::Channel> d_channel;
    std::shared_ptr<ByteStream::StubInterface> d_bytestreamClient;
    std::shared_ptr<ContentAddressableStorage::StubInterface> d_casClient;
    std::shared_ptr<Capabilities::StubInterface> d_capabilitiesClient;

    std::string d_uuid;

  public:
    Client() {}

    Client(std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
           std::shared_ptr<ContentAddressableStorage::StubInterface> casClient,
           std::shared_ptr<Capabilities::StubInterface> capabilitiesClient,
           int64_t maxBatchTotalSizeBytes = BYTESTREAM_CHUNK_SIZE)
        : d_bytestreamClient(bytestreamClient), d_casClient(casClient),
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
         std::shared_ptr<Capabilities::StubInterface> capabilitiesClient);

    /**
     * Download the blob with the given digest and return it.
     *
     * If the size of the received blob does not match the digest, throw an
     * exception.
     */
    std::string fetchString(const Digest &digest);

    /**
     * Download the blob with the given digest to the given file descriptor.
     *
     * If the file descriptor cannot be written to, or the size of the
     * received blob does not match the digest, throw an exception.
     */
    void download(int fd, const Digest &digest);

    /**
     * Upload the given string. If it can't be uploaded successfully, throw
     * an exception.
     */
    void upload(const std::string &str, const Digest &digest);

    /**
     * Upload a blob from the given file descriptor. If it can't be uploaded
     * successfully, throw an exception.
     */
    void upload(int fd, const Digest &digest);

    struct UploadRequest {
        Digest digest;
        std::string data;

        UploadRequest(const Digest &_digest, const std::string _data)
            : digest(_digest), data(_data){};
    };

    typedef std::vector<Digest> UploadResults;

    /* Upload multiple digests in an efficient way, allowing each digest to
     * potentially fail separately.
     *
     * Return a list containing the Digests that failed to be uploaded.
     * (An empty result indicates that all digests were uploaded.)
     */
    UploadResults uploadBlobs(const std::vector<UploadRequest> &requests);

    typedef std::unordered_map<std::string, std::string> DownloadedData;

    /* Given a list of digests, download the data and return it in a map
     * indexed by hash. Allow each digest to potentially fail separately.
     *
     * The hashes that could *not* be fetched will *not* be defined in the
     * returned map.
     */
    DownloadedData downloadBlobs(const std::vector<Digest> &digests);

    /**
     * Given a list of digests, creates and sends a `FindMissingBlobsRequest`
     * to the server.
     *
     * Returns a list of Digests that the remote server reports not having,
     * or throws a runtime_exception if the request failed.
     */
    std::vector<Digest> findMissingBlobs(const std::vector<Digest> &digests);

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
     * Upload the given Protocol Buffer message to CAS and return its Digest.
     */
    template <typename Msg> inline Digest uploadMessage(const Msg &msg)
    {
        const std::string str = msg.SerializeAsString();
        const Digest digest = CASHash::hash(str);
        this->upload(str, digest);
        return digest;
    }

    int64_t d_maxBatchTotalSizeBytes;

  private:
    /* Uploads the requests contained in the range [start_index, end_index).
     *
     * The sum of bytes of the data in this range MUST NOT exceed the maximum
     * batch size request allowed.
     */
    UploadResults batchUpload(const std::vector<UploadRequest> &requests,
                              const size_t start_index,
                              const size_t end_index);

    /* Downloads the data for the Digests stored in the range
     * [start_index, end_index) of the given vector.
     *
     * The sum of sizes inside the range MUST NOT exceed the maximum batch
     * size request allowed.
     */
    DownloadedData batchDownload(const std::vector<Digest> digests,
                                 const size_t start_index,
                                 const size_t end_index);

    /* Given a list of digests sorted by increasing size, forms batches
     * according to the value of `d_maxBatchTotalSizeBytes`.
     */
    std::vector<std::pair<size_t, size_t>>
    makeBatches(const std::vector<Digest> &digests);
};

} // namespace buildboxcommon

#endif
