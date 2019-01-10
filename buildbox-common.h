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

#ifndef BUILDBOX_COMMON_H
#define BUILDBOX_COMMON_H

#include "build/bazel/remote/execution/v2/remote_execution.grpc.pb.h"
#include "google/bytestream/bytestream.grpc.pb.h"

using namespace google::bytestream;
using namespace build::bazel::remote::execution::v2;

#define BUFFER_SIZE (1024 * 1024)

class Client {
  public:
    void init(const char *remote_url, const char *server_cert,
              const char *client_key, const char *client_cert);
    void download(int fd, const Digest &digest);
    void upload(int fd, const Digest &digest);
    bool batch_upload_add(const Digest &digest, const std::vector<char> &data);
    bool batch_upload();
    bool batch_download_add(const Digest &digest);
    bool batch_download_next(const Digest **digest, const std::string **data);

    int64_t max_batch_total_size_bytes;

  private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<ByteStream::Stub> bytestream_client;
    std::unique_ptr<ContentAddressableStorage::Stub> cas_client;
    std::unique_ptr<Capabilities::Stub> capabilities_client;

    std::string uuid;

    BatchUpdateBlobsRequest batch_update_request;
    BatchUpdateBlobsResponse batch_update_response;
    int64_t batch_update_size;

    std::unique_ptr<grpc::ClientContext> batch_read_context;
    BatchReadBlobsRequest batch_read_request;
    BatchReadBlobsResponse batch_read_response;
    BatchReadBlobsResponse_Response batch_read_blob_response;
    int64_t batch_read_size;
    bool batch_read_request_sent;
    int batch_read_response_index;
};

#endif
