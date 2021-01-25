// Copyright 2020 Bloomberg Finance L.P
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

#include <buildboxcommon_grpcretrier.h>

#include <buildboxcommon_logging.h>

#include <google/protobuf/util/time_util.h>
#include <google/rpc/error_details.grpc.pb.h>

#include <math.h>
#include <thread>

namespace {
std::string retryingInvocationWarningMessage(
    const std::string &grpcInvocationName, const grpc::Status &grpcError,
    const unsigned int attemptNumber, const unsigned int totalAttempts,
    const double &retryDelayMs)
{
    std::stringstream s;
    s << "Attempt " << attemptNumber + 1 << "/" << totalAttempts + 1;

    if (!grpcInvocationName.empty()) {
        s << " for \"" << grpcInvocationName << "\"";
    }

    s << " failed with gRPC error [" << grpcError.error_code() << ": "
      << grpcError.error_message() << "], "
      << "retrying in " << retryDelayMs << " ms...";

    return s.str();
}

std::string
retryAttemptsExceededErrorMessage(const std::string &grpcInvocationName,
                                  const grpc::Status &grpcError,
                                  const unsigned int retryLimit)
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
bool GrpcRetrier::issueRequest()
{
    d_retryAttempts = 0;

    while (true) {
        grpc::ClientContext context;
        if (d_metadataAttacher) {
            d_metadataAttacher(&context);
        }

        d_status = d_grpcInvocation(context);
        if (d_status.ok() || !statusIsRetryable(d_status)) {
            if (!d_status.ok()) {
                BUILDBOX_LOG_ERROR(d_grpcInvocationName + " failed with: " +
                                   std::to_string(d_status.error_code()) +
                                   ": " + d_status.error_message());
            }

            return true;
        }

        // The error might contain a `RetryInfo` message specifying a number of
        // seconds to wait before retrying. If so, use it for the base value.
        if (d_retryAttempts == 0 && !d_status.error_details().empty()) {
            google::rpc::RetryInfo retryInfo;
            if (retryInfo.ParseFromString(d_status.error_details())) {
                const google::protobuf::int64 serverDelay =
                    google::protobuf::util::TimeUtil::DurationToMilliseconds(
                        retryInfo.retry_delay());

                if (serverDelay > 0) {
                    d_retryDelayBase = std::chrono::milliseconds(serverDelay);
                    BUILDBOX_LOG_DEBUG("Overriding retry delay base with "
                                       "value specified by server: "
                                       << d_retryDelayBase.count() << " ms");
                }
            }
        }

        // The call failed and could be retryable on its own.
        if (d_retryAttempts >= d_retryLimit) {
            const std::string errorMessage = retryAttemptsExceededErrorMessage(
                d_grpcInvocationName, d_status, d_retryLimit);
            BUILDBOX_LOG_ERROR(errorMessage);

            return false;
        }

        // Delay the next call based on the number of attempts made:
        const auto retryDelay = d_retryDelayBase * pow(1.6, d_retryAttempts);
        BUILDBOX_LOG_WARNING(retryingInvocationWarningMessage(
            d_grpcInvocationName, d_status, d_retryAttempts, d_retryLimit,
            retryDelay.count()));

        std::this_thread::sleep_for(retryDelay);

        d_retryAttempts++;
    }
}

bool GrpcRetrier::statusIsRetryable(const grpc::Status &status) const
{
    return d_retryableStatusCodes.count(status.error_code());
}

GrpcRetrier GrpcRetrierFactory::makeRetrier(
    const GrpcRetrier::GrpcInvocation &grpcInvocation,
    const std::string &grpcInvocationName,
    const GrpcRetrier::GrpcStatusCodes &retryableStatusCodes) const
{
    GrpcRetrier retrier(d_retryLimit, d_retryDelayBase, grpcInvocation,
                        grpcInvocationName, retryableStatusCodes);

    if (d_metadataAttacher) {
        retrier.setMetadataAttacher(d_metadataAttacher);
    }

    return retrier;
}

} // namespace buildboxcommon
