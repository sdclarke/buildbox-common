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

#include <math.h>
#include <sstream>
#include <thread>

namespace buildboxcommon {

void grpcRetry(
    const std::function<grpc::Status(grpc::ClientContext &)> &grpcInvocation,
    int grpcRetryLimit, int grpcRetryDelay)
{
    int nAttempts = 0;
    grpc::Status status;
    do {
        grpc::ClientContext context;
        status = grpcInvocation(context);
        if (status.ok()) {
            return;
        }
        else {
            /* The call failed. */
            if (nAttempts < grpcRetryLimit) {
                /* Delay the next call based on the number of attempts made */
                int timeDelay = static_cast<int>(
                    grpcRetryDelay * pow(static_cast<double>(2), nAttempts));

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
    } while (nAttempts < grpcRetryLimit + 1);

    throw std::runtime_error("Retry limit exceeded. Last gRPC error was " +
                             std::to_string(status.error_code()) + ": " +
                             status.error_message());
}

} // namespace buildboxcommon
