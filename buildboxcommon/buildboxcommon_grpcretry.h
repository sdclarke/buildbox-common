// Copyright 2018 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_BUILDBOXCOMMON_GRPCRETRY
#define INCLUDED_BUILDBOXCOMMON_GRPCRETRY

#include <buildboxcommon_protos.h>
#include <buildboxcommon_requestmetadata.h>

#include <functional>

namespace buildboxcommon {

class GrpcError : public std::runtime_error {
  public:
    explicit GrpcError(const char *message, const grpc::Status &_status)
        : std::runtime_error(message), status(_status)
    {
    }
    explicit GrpcError(const std::string &message, const grpc::Status &_status)
        : std::runtime_error(message), status(_status)
    {
    }

    grpc::Status status;
};

/**
 * Call a GRPC method. On failure, retry up to RECC_RETRY_LIMIT times,
 * using binary exponential backoff to delay between calls.
 *
 * As input, takes a function that takes a grpc::ClientContext and returns a
 * grpc::Status.
 *
 * In the second form, also takes a function that can preprocess the context by
 * attaching metadata, setting a timeout, etc.
 *
 */

void grpcRetry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    int grpcRetryLimit, int grpcRetryDelay);

void grpcRetry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    int grpcRetryLimit, int grpcRetryDelay,
    const std::function<void(grpc::ClientContext *)> &metadataAttacher);

} // namespace buildboxcommon

#endif
