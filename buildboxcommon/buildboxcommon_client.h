// buildboxcommon_client.h                                            -*-C++-*-
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
//@PURPOSE: Communicate with Content-Addressable Storage servers.
//
//@CLASSES:
//  buildboxcommon::Client: CAS client class wrapping gRPC stubs
//
//@DESCRIPTION: This component provides a client implementation of the
// Content-Addressable Storage API defined by Bazel's Remote Execution
// API. It can be used to fetch and upload blobs to/from remote servers by
// directly communicating with a remote server (i.e. without caching).
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Example 1: Downloading a blob from CAS to a file
/// - - - - - - - - - - - - - - - - - - - - - - - -
// To download a blob from CAS to a file, construct a Client, call 'init' with
// the server URL (and, optionally, keys and certificates for secure access),
// then call 'download' passing the file descriptor to download to and the
// digest of the blob you wish to download.

#include <build/bazel/remote/execution/v2/remote_execution.grpc.pb.h>
#include <google/bytestream/bytestream.grpc.pb.h>

namespace BloombergLP {
namespace buildboxcommon {

using namespace google::bytestream;
using namespace build::bazel::remote::execution::v2;

// ============
// class Client
// ============

class Client {
    // This class implements a mechanism to communicate with remote CAS
    // servers. It includes data members which keep track of an ongoing
    // batch upload or batch download request.
    //
    // This class is *exception* *neutral* with no guarantee of rollback:
    // If an exception is thrown during the invocation of a method on a
    // pre-existing object, the object is left in a valid state, but its
    // value is undefined. In no event is memory leaked.

  private:
    // DATA
    std::shared_ptr<grpc::Channel> d_channel;
    std::unique_ptr<ByteStream::Stub> d_bytestreamClient;
    std::unique_ptr<ContentAddressableStorage::Stub> d_casClient;
    std::unique_ptr<Capabilities::Stub> d_capabilitiesClient;

    std::string d_uuid; // UUID sent with upload requests

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

    int64_t d_maxBatchTotalSizeBytes;

  public:
    // MANIPULATORS
    void init(const char *remoteURL, const char *serverCert = nullptr,
              const char *clientKey = nullptr,
              const char *clientCert = nullptr);
    // Connect to the CAS server with the specified 'remoteURL'.
    // Optionally specify a 'serverCert', 'clientKey', and 'clientCert'
    // to configure a secure connection.

    void download(int fd, const Digest &digest);
    // Download the blob with the specified `digest` to the specified `fd`.
    // Throw an exception if `fd` cannot be written to or if the size of
    // the received blob does not match the `digest`.

    void upload(int fd, const Digest &digest);
    // Upload a blob from the specified `fd`. Throw an exception if the
    // blob cannot be uploaded successfully.

    bool batchUploadAdd(const Digest &digest, const std::vector<char> &data);
    // Add the specified digest and data to the current batch upload request.
    // Return false if the additional data would cause the batch upload's size
    // to exceed the maximum, and true otherwise.

    bool batchUpload();
    // Send the current batch upload request to the server. Return true if all
    // requests in the batch succeeded, and throw an exception otherwise.
    // After the request is sent, the client's batch upload state is reset so
    // a subsequent call to `batchUpload` would send an empty request.

    bool batchDownloadAdd(const Digest &digest);
    // Add the specified digest to the current batch download request. Return
    // false if the additional data would cause the batch download response's
    // size to exceed the maximum, and true otherwise.

    bool batchDownloadNext(const Digest **digest, const std::string **data);
    // Send the current batch download request to the server if it hasn't been
    // sent already. Load a digest and its corresponding data from the response
    // into the specified `digest` and `data` and return true. If there are no
    // entries left in the current batch, return false without modifying
    // `digest` or `data`.

    void setMaxBatchTotalSizeBytes(int64_t maxBatchTotalSizeBytes);
    // Set the maximum total size of a batch upload or download.

    // ACCESSORS
    int64_t maxBatchTotalSizeBytes() const;
    // Return the maximum total size of a batch upload or download.
};

//============================================================================
//                 INLINE DEFINITIONS
//============================================================================
// MANIPULATORS
inline void Client::setMaxBatchTotalSizeBytes(int64_t maxBatchTotalSizeBytes)
{
    this->d_maxBatchTotalSizeBytes = maxBatchTotalSizeBytes;
}
// ACCESSORS
inline int64_t Client::maxBatchTotalSizeBytes() const
{
    return this->d_maxBatchTotalSizeBytes;
}

} // close package namespace
} // close enterprise namespace

#endif
