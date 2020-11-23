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

#include <algorithm>
#include <math.h>
#include <sstream>
#include <thread>

namespace {

std::string retryingInvocationWarningMessage(
    const std::string &grpcInvocationName, const grpc::Status &grpcError,
    const int attemptNumber, const int totalAttempts, const int retryDelay)
{
    std::stringstream s;
    s << "Attempt " << attemptNumber + 1 << "/" << totalAttempts + 1;

    if (!grpcInvocationName.empty()) {
        s << " for \"" << grpcInvocationName << "\"";
    }

    s << " failed with gRPC error [" << grpcError.error_code() << ": "
      << grpcError.error_message() << "], "
      << "retrying in " << retryDelay << " ms...";

    return s.str();
}

std::string
retryAttemptsExceededErrorMessage(const std::string &grpcInvocationName,
                                  const grpc::Status &grpcError,
                                  const int retryLimit)
{
    std::stringstream s;
    s << "Retry limit (" << retryLimit << ") exceeded";
    if (!grpcInvocationName.empty()) {
        s << " for \"" << grpcInvocationName << "\"";
    }

    s << ", last gRPC error was [" << grpcError.error_code() << ": "
      << grpcError.error_message() << "]";

    return s.str();
}
} // namespace

namespace buildboxcommon {

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    const int grpcRetryLimit, const int grpcRetryDelay)
{
    GrpcRetry::retry(grpcInvocation, "", grpcRetryLimit, grpcRetryDelay,
                     [](grpc::ClientContext *) { return; });
}

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    const std::string &grpcInvocationName, const int grpcRetryLimit,
    const int grpcRetryDelay)
{
    GrpcRetry::retry(grpcInvocation, grpcInvocationName, grpcRetryLimit,
                     grpcRetryDelay, [](grpc::ClientContext *) { return; });
}

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    const int grpcRetryLimit, const int grpcRetryDelay,
    const std::function<void(grpc::ClientContext *)> &metadataAttacher)
{
    return retry(grpcInvocation, "", grpcRetryLimit, grpcRetryDelay,
                 metadataAttacher);
}

void GrpcRetry::retry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    const std::string &grpcInvocationName, const int grpcRetryLimit,
    const int grpcRetryDelay,
    const std::function<void(grpc::ClientContext *)> &metadataAttacher,
    GrpcStatusCodes errorsToRetryOn)
{
    int nAttempts = 0;
    grpc::Status status;

    // We always retry on UNAVAILABLE
    errorsToRetryOn.insert(grpc::StatusCode::UNAVAILABLE);

    do {
        grpc::ClientContext context;
        metadataAttacher(&context);

        status = grpcInvocation(context);
        if (status.ok()) {
            return;
        }
        else if (std::find(errorsToRetryOn.begin(), errorsToRetryOn.end(),
                           status.error_code()) != errorsToRetryOn.end()) {
            /* The call failed and is retryable on its own. */
            if (nAttempts < grpcRetryLimit) {
                /* Delay the next call based on the number of attempts made */
                const int timeDelay =
                    static_cast<int>(grpcRetryDelay * pow(1.6, nAttempts));

                BUILDBOX_LOG_WARNING(retryingInvocationWarningMessage(
                    grpcInvocationName, status, nAttempts, grpcRetryLimit,
                    timeDelay));

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

    const std::string errorMessage = retryAttemptsExceededErrorMessage(
        grpcInvocationName, status, grpcRetryLimit);
    BUILDBOX_LOG_ERROR(errorMessage);
    throw GrpcError(errorMessage, status);
}

} // namespace buildboxcommon
