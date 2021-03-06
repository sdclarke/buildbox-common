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

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_reloadtokenauthenticator.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace buildboxcommon {

namespace {
static const char *HTTP_PREFIX = "http://";
static const char *HTTPS_PREFIX = "https://";
static const char *GRPC_PREFIX = "grpc://";
static const char *GRPCS_PREFIX = "grpcs://";
static const char *UNIX_SOCKET_PREFIX = "unix:";

static void printPadded(int padWidth, const std::string &str)
{
    char previousFill = std::cerr.fill(' ');
    std::ios_base::fmtflags previousFlags(std::cerr.flags());
    std::cerr << "    ";
    std::cerr << std::left << std::setw(padWidth - 5) << str;
    std::cerr << " ";
    std::cerr.flags(previousFlags);
    std::cerr.fill(previousFill);
}

inline const char *safeStream(const char *var) { return (var ? var : "null"); }

} // namespace

void ConnectionOptions::setClientCert(const std::string &value)
{
    this->d_clientCert = value.c_str();
}

void ConnectionOptions::setServerCertPath(const std::string &value)
{
    this->d_serverCertPath = value.c_str();
}

void ConnectionOptions::setClientKey(const std::string &value)
{
    this->d_clientKey = value.c_str();
}

void ConnectionOptions::setClientKeyPath(const std::string &value)
{
    this->d_clientKeyPath = value.c_str();
}

void ConnectionOptions::setAccessTokenPath(const std::string &value)
{
    this->d_accessTokenPath = value.c_str();
}

void ConnectionOptions::setTokenReloadInterval(const std::string &value)
{
    this->d_tokenReloadInterval = value.c_str();
}

void ConnectionOptions::setInstanceName(const std::string &value)
{
    this->d_instanceName = value.c_str();
}

void ConnectionOptions::setRetryDelay(const std::string &value)
{
    this->d_retryDelay = value.c_str();
}

void ConnectionOptions::setRetryLimit(const std::string &value)
{
    this->d_retryLimit = value.c_str();
}

void ConnectionOptions::setServerCert(const std::string &value)
{
    this->d_serverCert = value.c_str();
}

void ConnectionOptions::setClientCertPath(const std::string &value)
{
    this->d_clientCertPath = value.c_str();
}

void ConnectionOptions::setUrl(const std::string &value)
{
    this->d_url = value.c_str();
}

void ConnectionOptions::setUseGoogleApiAuth(const bool value)
{
    this->d_useGoogleApiAuth = value;
}

void ConnectionOptions::setLoadBalancingPolicy(const std::string &value)
{
    this->d_loadBalancingPolicy = value.c_str();
}

bool ConnectionOptions::parseArg(const char *arg, const char *prefix)
{
    if (arg == nullptr || arg[0] != '-' || arg[1] != '-') {
        return false;
    }
    arg += 2;
    if (prefix != nullptr) {
        const size_t prefixLen = strlen(prefix);
        if (strncmp(arg, prefix, prefixLen) != 0) {
            return false;
        }
        arg += prefixLen;
    }
    const char *assign = strchr(arg, '=');
    if (assign) {
        const std::string key(arg, static_cast<size_t>(assign - arg));
        const char *value = assign + 1;
        if (key == "remote") {
            this->d_url = value;
            return true;
        }
        else if (key == "instance") {
            this->d_instanceName = value;
            return true;
        }
        else if (key == "server-cert") {
            this->d_serverCertPath = value;
            return true;
        }
        else if (key == "client-key") {
            this->d_clientKeyPath = value;
            return true;
        }
        else if (key == "client-cert") {
            this->d_clientCertPath = value;
            return true;
        }
        else if (key == "access-token") {
            this->d_accessTokenPath = value;
            return true;
        }
        else if (key == "retry-limit") {
            this->d_retryLimit = value;
            return true;
        }
        else if (key == "retry-delay") {
            this->d_retryDelay = value;
            return true;
        }
        else if (key == "token-reload-interval") {
            this->d_tokenReloadInterval = value;
            return true;
        }
        else if (key == "load-balancing-policy") {
            this->d_loadBalancingPolicy = value;
            return true;
        }
    }
    else if (std::string(arg) == "googleapi-auth") {
        this->d_useGoogleApiAuth = true;
        return true;
    }
    return false;
}

void ConnectionOptions::putArgs(std::vector<std::string> *out,
                                const char *prefix) const
{
    const std::string p(prefix == nullptr ? "" : prefix);
    if (this->d_url != nullptr) {
        out->push_back("--" + p + "remote=" + std::string(this->d_url));
    }
    if (this->d_instanceName != nullptr) {
        out->push_back("--" + p +
                       "instance=" + std::string(this->d_instanceName));
    }
    if (this->d_serverCertPath != nullptr) {
        out->push_back("--" + p +
                       "server-cert=" + std::string(this->d_serverCertPath));
    }
    if (this->d_clientKeyPath != nullptr) {
        out->push_back("--" + p +
                       "client-key=" + std::string(this->d_clientKeyPath));
    }
    if (this->d_clientCertPath != nullptr) {
        out->push_back("--" + p +
                       "client-cert=" + std::string(this->d_clientCertPath));
    }
    if (this->d_accessTokenPath != nullptr) {
        out->push_back("--" + p +
                       "access-token=" + std::string(this->d_accessTokenPath));
    }
    if (this->d_tokenReloadInterval != nullptr) {
        out->push_back("--" + p + "token-reload-interval=" +
                       std::string(this->d_tokenReloadInterval));
    }
    if (this->d_retryLimit != nullptr) {
        out->push_back("--" + p +
                       "retry-limit=" + std::string(this->d_retryLimit));
    }
    if (this->d_retryDelay != nullptr) {
        out->push_back("--" + p +
                       "retry-delay=" + std::string(this->d_retryDelay));
    }
    if (this->d_useGoogleApiAuth) {
        out->push_back("--googleapi-auth");
    }
    if (this->d_loadBalancingPolicy != nullptr) {
        out->push_back("--" + p + "load-balancing-policy=" +
                       std::string(this->d_loadBalancingPolicy));
    }
}

std::shared_ptr<grpc::Channel> ConnectionOptions::createChannel() const
{
    BUILDBOX_LOG_DEBUG("Creating grpc channel to [" << this->d_url << "]");
    std::string target;
    std::shared_ptr<grpc::ChannelCredentials> creds;
    grpc::ChannelArguments channel_args;
    bool secure = false;

    if (strncmp(this->d_url, HTTP_PREFIX, strlen(HTTP_PREFIX)) == 0) {
        target = this->d_url + strlen(HTTP_PREFIX);
    }
    else if (strncmp(this->d_url, GRPC_PREFIX, strlen(GRPC_PREFIX)) == 0) {
        target = this->d_url + strlen(GRPC_PREFIX);
    }
    else if (strncmp(this->d_url, HTTPS_PREFIX, strlen(HTTPS_PREFIX)) == 0) {
        target = this->d_url + strlen(HTTPS_PREFIX);
        secure = true;
    }
    else if (strncmp(this->d_url, GRPCS_PREFIX, strlen(GRPCS_PREFIX)) == 0) {
        target = this->d_url + strlen(GRPCS_PREFIX);
        secure = true;
    }
    else if (strncmp(this->d_url, UNIX_SOCKET_PREFIX,
                     strlen(UNIX_SOCKET_PREFIX)) == 0) {
        target = this->d_url;
    }
    else {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Unsupported URL scheme");
    }
    if (secure) {
        auto options = grpc::SslCredentialsOptions();
        if (this->d_serverCert) {
            options.pem_root_certs = this->d_serverCert;
        }
        else if (this->d_serverCertPath) {
            options.pem_root_certs =
                FileUtils::getFileContents(this->d_serverCertPath);
        }
        if (this->d_clientKey) {
            options.pem_private_key = this->d_clientKey;
        }
        else if (this->d_clientKeyPath) {
            options.pem_private_key =
                FileUtils::getFileContents(this->d_clientKeyPath);
        }
        if (this->d_clientCert) {
            options.pem_cert_chain = this->d_clientCert;
        }
        else if (this->d_clientCertPath) {
            options.pem_cert_chain =
                FileUtils::getFileContents(this->d_clientCertPath);
        }

        if (this->d_accessTokenPath && this->d_useGoogleApiAuth) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error,
                "Cannot use both Access Token Auth and GoogleAPIAuth.");
        }
        else if (this->d_useGoogleApiAuth) {
            creds = grpc::GoogleDefaultCredentials();
            if (!creds) {
                throw std::runtime_error(
                    "Failed to initialize GoogleAPIAuth. Make Sure you have a "
                    "token and have set the appropriate environment variable "
                    "[GOOGLE_APPLICATION_CREDENTIALS] as necessary.");
            }
        }
        else {
            // Set-up SSL creds
            creds = grpc::SslCredentials(options);

            // Include access token, if configured
            if (this->d_accessTokenPath) {
                // It is important to do this last, after first creating the
                // `grpc::SslCredentials()` so that we can use them in the
                // constructor of `grpc::CompositeChannelCredentials`
                creds = grpc::SslCredentials(options);

                std::shared_ptr<grpc::CallCredentials> call_creds =
                    grpc::MetadataCredentialsFromPlugin(
                        std::make_unique<ReloadTokenAuthenticator>(
                            this->d_accessTokenPath,
                            this->d_tokenReloadInterval));
                // Wrap both channel and call creds together
                // so that all requests on this channel use both
                // the Channel and Call Creds
                creds = grpc::CompositeChannelCredentials(creds, call_creds);
            }
        }
    }
    if (!creds) {
        // Secure creds weren't set, use default insecure
        creds = grpc::InsecureChannelCredentials();
        // If any of the secure-only options were specified with an insecure
        // endpoint, throw
        if (this->d_serverCert || this->d_serverCertPath ||
            this->d_clientKey || this->d_clientKeyPath || this->d_clientCert ||
            this->d_clientCertPath || this->d_accessTokenPath ||
            this->d_useGoogleApiAuth) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error,
                "Secure Channel options cannot be used with this URL");
        }
    }

    if (this->d_loadBalancingPolicy) {
        channel_args.SetLoadBalancingPolicyName(this->d_loadBalancingPolicy);
    }
    return grpc::CreateCustomChannel(target, creds, channel_args);
}

void ConnectionOptions::printArgHelp(int padWidth, const char *serviceName,
                                     const char *prefix)
{
    std::string p(prefix == nullptr ? "" : prefix);

    printPadded(padWidth, "--" + p + "remote=URL");
    std::clog << "URL for " << serviceName << " service\n";

    printPadded(padWidth, "--" + p + "instance=NAME");
    std::clog << "Name of the " << serviceName << " instance\n";

    printPadded(padWidth, "--" + p + "server-cert=PATH");
    std::clog << "Public server certificate for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-key=PATH");
    std::clog << "Private client key for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-cert=PATH");
    std::clog << "Public client certificate for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "access-token=PATH");
    std::clog << "Access Token for authentication "
                 "(e.g. JWT, OAuth access token, etc), "
                 "will be included as an HTTP Authorization bearer token.\n";

    printPadded(padWidth, "--" + p + "token-reload-interval=MINUTES");
    std::clog
        << "Time to wait before refreshing access token from disk again. "
           "The following suffixes can be optionally specified: "
           "M (minutes), H (hours). "
           "Value defaults to minutes if suffix not specified.\n";

    printPadded(padWidth, "--" + p + "googleapi-auth");
    std::clog << "Use GoogleAPIAuth when this flag is set.\n";

    printPadded(padWidth, "--" + p + "retry-limit=INT");
    std::clog << "Number of times to retry on grpc errors\n";

    printPadded(padWidth, "--" + p + "retry-delay=MILLISECONDS");
    std::clog << "How long to wait before the first grpc retry\n";

    printPadded(padWidth, "--" + p + "load-balancing-policy");
    std::clog << "Which grpc load balancing policy to use. "
                 "Valid options are 'round_robin' and 'grpclb'\n";
}

std::ostream &operator<<(std::ostream &out, const ConnectionOptions &obj)
{
    out << "url = \"" << safeStream(obj.d_url) << "\", instance = \""
        << safeStream(obj.d_instanceName) << "\", serverCert = \""
        << safeStream(obj.d_serverCert) << "\", serverCertPath = \""
        << safeStream(obj.d_serverCertPath) << "\", clientKey = \""
        << safeStream(obj.d_clientKey) << "\", clientKeyPath = \""
        << safeStream(obj.d_clientKeyPath) << "\", clientCert = \""
        << safeStream(obj.d_clientCert) << "\", clientCertPath = \""
        << safeStream(obj.d_clientCertPath) << "\", accessTokenPath = \""
        << safeStream(obj.d_accessTokenPath)
        << "\", token-reload-interval = \""
        << safeStream(obj.d_tokenReloadInterval)
        << "\", googleapi-auth = " << std::boolalpha << obj.d_useGoogleApiAuth
        << ", retry-limit = \"" << safeStream(obj.d_retryLimit)
        << "\", retry-delay = \"" << safeStream(obj.d_retryDelay) << "\""
        << "\", load-balancing-policy = \""
        << safeStream(obj.d_loadBalancingPolicy) << "\"";

    return out;
}

} // namespace buildboxcommon
