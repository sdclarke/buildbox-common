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

#ifndef INCLUDED_BUILDBOXCOMMON_CONNECTIONOPTIONS
#define INCLUDED_BUILDBOXCOMMON_CONNECTIONOPTIONS

#include <grpcpp/channel.h>
#include <memory>
#include <string>
#include <vector>

namespace buildboxcommon {

struct ConnectionOptions {
    const char *d_clientCert = nullptr;
    const char *d_clientCertPath = nullptr;
    const char *d_clientKey = nullptr;
    const char *d_clientKeyPath = nullptr;
    const char *d_accessTokenPath = nullptr;
    const char *d_instanceName = nullptr;
    const char *d_serverCert = nullptr;
    const char *d_serverCertPath = nullptr;
    const char *d_url = nullptr;
    bool d_useGoogleApiAuth = false;
    const char *d_tokenReloadInterval = nullptr;

    /*
     * These are strings to allow for easier
     * propagation from the worker to the runner.
     */
    const char *d_retryLimit = "4";    /* Number of times to retry */
    const char *d_retryDelay = "1000"; /* Initial delay in milliseconds */

    /**
     * If the given argument is a server option, update this struct with
     * it and return true. Otherwise, return false.
     *
     * Valid server options are "--remote=URL", "--instance=NAME",
     * "--server-cert=PATH", "--client-key=PATH", "--client-cert=PATH", and,
     * "--access-token=PATH"
     *
     * If a prefix is passed, it's added to the name of each option.
     * (For example, passing a prefix of "cas-" would cause this method to
     * recognize "--cas-remote=URL" and so on.)
     */
    bool parseArg(const char *arg, const char *prefix = nullptr);

    void setClientCert(const std::string &value);
    void setClientCertPath(const std::string &value);
    void setClientKey(const std::string &value);
    void setClientKeyPath(const std::string &value);
    void setAccessTokenPath(const std::string &value);
    void setTokenReloadInterval(const std::string &value);
    void setInstanceName(const std::string &value);
    void setRetryDelay(const std::string &value);
    void setRetryLimit(const std::string &value);
    void setServerCert(const std::string &value);
    void setServerCertPath(const std::string &value);
    void setUrl(const std::string &value);
    void setUseGoogleApiAuth(const bool value);

    /**
     * Add arguments corresponding to this struct's settings to the given
     * vector. If a prefix is passed, it's added to the name of each
     * option as in parseArg.
     */
    void putArgs(std::vector<std::string> *out,
                 const char *prefix = nullptr) const;

    /**
     * Create a gRPC Channel from the options in this struct.
     */
    std::shared_ptr<grpc::Channel> createChannel() const;

    /**
     * Print usage-style help messages for each of the arguments parsed
     * by ConnectionOptions.
     *
     * padWidth is the column to align the argument descriptions to,
     * serviceName is a human-readable name to be used in the "URL for
     * [serviceName] service" help message, and the prefix is added to
     * the name of each option as in parseArg.
     */
    static void printArgHelp(int padWidth, const char *serviceName = "CAS",
                             const char *prefix = nullptr);
};

std::ostream &operator<<(std::ostream &out, const ConnectionOptions &obj);

} // namespace buildboxcommon
#endif
