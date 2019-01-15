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

#include <buildboxcommon_connectionoptions.h>

#include <build/bazel/remote/execution/v2/remote_execution.grpc.pb.h>
#include <google/bytestream/bytestream.grpc.pb.h>

namespace buildboxcommon {

using namespace google::bytestream;
using namespace build::bazel::remote::execution::v2;

/**
 * Implements a mechanism to communicate with remote CAS servers, and includes
 * data members to keep track of an ongoing batch upload or batch download
 * request.
 */
class Client {
  private:
    std::shared_ptr<grpc::Channel> d_channel;
    std::unique_ptr<ByteStream::Stub> d_bytestreamClient;
    std::unique_ptr<ContentAddressableStorage::Stub> d_casClient;
    std::unique_ptr<Capabilities::Stub> d_capabilitiesClient;

    std::string d_uuid;

    BatchUpdateBlobsRequest d_batchUpdateRequest;
    BatchUpdateBlobsResponse d_batchUpdateResponse;
    int64_t d_batchUpdateSize;

    std::unique_ptr<grpc::ClientContext> d_batchReadContext;
    BatchReadBlobsRequest d_batchReadRequest;
    BatchReadBlobsResponse d_batchReadResponse;
    BatchReadBlobsResponse_Response d_batchReadBlobResponse;
    int64_t d_batchReadSize;
    bool d_batchReadRequestSent;
    int d_batchReadResponseIndex;

  public:
    /**
     * Connect to the CAS server with the given connection options.
     */
    void init(const ConnectionOptions &options);

    /**
     * Download the blob with the given digest to the given file descriptor.
     *
     * If the file descriptor cannot be written to, or the size of the
     * received blob does not match the digest, throw an exception.
     */
    void download(int fd, const Digest &digest);

    /**
     * Upload a blob from the given file descriptor. If it can't be uploaded
     * successfully, throw an exception.
     */
    void upload(int fd, const Digest &digest);

    /**
     * Add the specified digest and data to the current batch upload request
     * and return true. If the additional data would cause the batch upload's
     * size to exceed the maximum, return false instead of adding it.
     */
    bool batchUploadAdd(const Digest &digest, const std::vector<char> &data);

    /**
     * Send the current batch upload request to the server, and clear the
     * state so a new batch upload request can be prepared/sent.
     *
     * Returns true if all requests in the batch succeeded and false
     * otherwise.
     */
    bool batchUpload();

    /**
     * Add the specified digest to the current batch download request and
     * return true. If the additional data would cause the batch download
     * response's size to exceed the maximum, return false instead of adding
     * it.
     */
    bool batchDownloadAdd(const Digest &digest);

    /**
     * Send the current batch download request to the server if it hasn't been
     * sent already, store pointers to a digest and its corresponding data in
     * the given `digest` and `data`, and return true.
     *
     * If there are no entries left in the current batch, this returns false
     * instead of modifying `digest` or `data`.
     */
    bool batchDownloadNext(const Digest **digest, const std::string **data);

    int64_t d_maxBatchTotalSizeBytes;
};

} // namespace buildboxcommon

#endif
