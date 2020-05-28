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

#include <buildboxcommon_grpcretry.h>

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_requestmetadata.h>

#include <math.h>
#include <sstream>
#include <thread>

namespace buildboxcommon {

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    int grpcRetryLimit, int grpcRetryDelay)
{
    GrpcRetry::retry(grpcInvocation, grpcRetryLimit, grpcRetryDelay,
                     [](grpc::ClientContext *) { return; });
}

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    int grpcRetryLimit, int grpcRetryDelay,
    const std::function<void(grpc::ClientContext *)> &metadataAttacher)
{
    int nAttempts = 0;
    grpc::Status status;
    do {
        grpc::ClientContext context;
        metadataAttacher(&context);

        status = grpcInvocation(context);
        if (status.ok()) {
            return;
        }
        else if (status.error_code() == grpc::StatusCode::UNAVAILABLE) {
            /* The call failed and is retryable on its own. */
            if (nAttempts < grpcRetryLimit) {
                /* Delay the next call based on the number of attempts made */
                const int timeDelay =
                    static_cast<int>(grpcRetryDelay * pow(1.6, nAttempts));

                std::ostringstream errorMsg;
                errorMsg << "Attempt " << nAttempts + 1 << "/"
                         << grpcRetryLimit + 1 << " failed with gRPC error "
                         << status.error_code() << ": "
                         << status.error_message() << ". Retrying in "
                         << timeDelay << " ms...";

                BUILDBOX_LOG_ERROR(errorMsg.str());

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(timeDelay));
            }
            nAttempts++;
        }
        else {
            /* The call failed and is not retryable on its own. */
            throw GrpcError(std::to_string(status.error_code()) + ": " +
                                status.error_message(),
                            status);
        }
    } while (nAttempts < grpcRetryLimit + 1);

    throw GrpcError("Retry limit exceeded. Last gRPC error was " +
                        std::to_string(status.error_code()) + ": " +
                        status.error_message(),
                    status);
}

} // namespace buildboxcommon
