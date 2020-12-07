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

#ifndef INCLUDED_BUILDBOXCOMMON_GRPCRETRIER
#define INCLUDED_BUILDBOXCOMMON_GRPCRETRIER

#include <buildboxcommon_protos.h>
#include <buildboxcommon_requestmetadata.h>

#include <buildboxcommon_connectionoptions.h>

#include <chrono>
#include <functional>
#include <set>
#include <string>

namespace buildboxcommon {

class GrpcRetrier final {

    /* This class wraps a function that issues a gRPC request and will attempt
     * to invoke the request until it either succeeds, fails with a final,
     * non-retryable error, or the limit of attempts was exceeded.
     *
     * Between retry attempts it will use an exponential-backoff delay.
     *
     * Sample usage:
     *
     * ```
     * const unsigned int retryLimit = 3;
     * const std::chrono::milliseconds retryDelayBase(100);
     *
     * GrpcRetrier r(retryLimit, retryDelayBase, grpcInvocationFunction,
     *               "grpcInvocationName()");
     *
     * if (r.issueRequest()) {
     *    // Received a final answer before exceeding the retry limit.
     *
     *    if (r.status().ok()) {
     *        // The gRPC request was successful.
     *    }
     *    else {
     *        // The request failed with a final error, such as `NOT_FOUND`.
     *    }
     *
     * }
     * else {
     *    // Retry limit exceeded; gRPC request failed.
     * }
     * ```
     *
     */

  public:
    typedef std::function<grpc::Status(grpc::ClientContext &)> GrpcInvocation;

    typedef std::function<void(grpc::ClientContext *)> MetadataAttacher;

    typedef std::set<grpc::StatusCode> GrpcStatusCodes;

    GrpcRetrier(const unsigned int retryLimit,
                const std::chrono::milliseconds &retryDelayBase,
                const GrpcInvocation &grpcInvocation,
                const std::string &grpcInvocationName)
        : GrpcRetrier(retryLimit, retryDelayBase, grpcInvocation,
                      grpcInvocationName, {})
    {
    }

    GrpcRetrier(const unsigned int retryLimit,
                const std::chrono::milliseconds &retryDelayBase,
                const GrpcInvocation &grpcInvocation,
                const std::string &grpcInvocationName,
                const GrpcStatusCodes &retryableStatusCodes)
        : d_grpcInvocation(grpcInvocation),
          d_grpcInvocationName(grpcInvocationName), d_retryLimit(retryLimit),
          d_retryDelayBase(retryDelayBase),
          d_retryableStatusCodes(retryableStatusCodes),
          d_metadataAttacher(nullptr)
    {
        // Always retry on UNAVAILABLE
        d_retryableStatusCodes.insert(grpc::StatusCode::UNAVAILABLE);
    }

    /* Returns the maximum number of retries that will be attempted after
     * an initial request that fails.
     */
    const unsigned int &retryLimit() const { return d_retryLimit; }

    /* Returns the value used as a base for the exponential-backoff wait
     * between attempts.
     */
    const std::chrono::milliseconds &retryDelayBase() const
    {
        return d_retryDelayBase;
    }

    /* Issue the gRPC request and return whether the request was completed in
     * fewer retries than the limit. (Note that the request might have failed
     * with a non-retryable status.)
     *
     * If the retry count was exceeded, this method returns `false`.
     *
     * `status()` will return the last value returned by the gRPC request.
     */
    bool issueRequest();

    /* Set of codes that enable to retry the request.  (Should contain errors
     * that are transient.)
     */
    const GrpcStatusCodes &retryableStatusCodes() const
    {
        return d_retryableStatusCodes;
    }

    void setMetadataAttacher(const MetadataAttacher &metadataAttacher)
    {
        d_metadataAttacher = metadataAttacher;
    }

    /* Return the `grpc::Status` received on the last attempt. */
    grpc::Status status() const { return d_status; }

    /* Number of retries attempted in `issueRequest()` */
    const unsigned int &retryAttempts() const { return d_retryAttempts; }

  private:
    // gRPC callback to perform a request and its human-readable name for logs:
    const GrpcInvocation d_grpcInvocation;
    const std::string d_grpcInvocationName;

    // Maximum number of attempts and delay between retries:
    const unsigned int d_retryLimit;
    const std::chrono::milliseconds d_retryDelayBase;

    // Status codes to retry:
    GrpcStatusCodes d_retryableStatusCodes;

    // Optional callback to attach metadata to the request before issuing it:
    MetadataAttacher d_metadataAttacher;

    // Results after `issueRequest()`:
    grpc::Status d_status;        // Last status received from the server.
    unsigned int d_retryAttempts; // Number of retries performed (excluding
                                  // original request):

    // Returns whether the status can be retried or should be considered final.
    bool statusIsRetryable(const grpc::Status &status) const;
};

class GrpcRetrierFactory final {
    /*
     * This class allows to easily construct `GrpcRetrier` instances by
     * configuring the global values once and only passing request-specific
     * arguments on each instantiation.
     */
  public:
    GrpcRetrierFactory(const unsigned int retryLimit,
                       const std::chrono::milliseconds &retryDelayBase)
        : GrpcRetrierFactory(retryLimit, retryDelayBase, nullptr)
    {
    }

    GrpcRetrierFactory(const unsigned int retryLimit,
                       const std::chrono::milliseconds &retryDelayBase,
                       const GrpcRetrier::MetadataAttacher &metadataAttacher)
        : d_retryLimit(retryLimit), d_retryDelayBase(retryDelayBase),
          d_metadataAttacher(metadataAttacher)
    {
    }

    GrpcRetrier makeRetrier(
        const GrpcRetrier::GrpcInvocation &grpcInvocation,
        const std::string &grpcInvocationName,
        const GrpcRetrier::GrpcStatusCodes &retryableStatusCodes = {}) const;

  private:
    const unsigned int d_retryLimit;
    const std::chrono::milliseconds d_retryDelayBase;

    const GrpcRetrier::MetadataAttacher d_metadataAttacher;
};

} // namespace buildboxcommon

#endif
