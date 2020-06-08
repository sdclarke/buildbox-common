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
        else if (key == "retry-limit") {
            this->d_retryLimit = value;
            return true;
        }
        else if (key == "retry-delay") {
            this->d_retryDelay = value;
            return true;
        }
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
    if (this->d_retryLimit != nullptr) {
        out->push_back("--" + p +
                       "retry-limit=" + std::string(this->d_retryLimit));
    }
    if (this->d_retryDelay != nullptr) {
        out->push_back("--" + p +
                       "retry-delay=" + std::string(this->d_retryDelay));
    }
}

std::shared_ptr<grpc::Channel> ConnectionOptions::createChannel() const
{
    BUILDBOX_LOG_DEBUG("Creating grpc channel to " << this->d_url);
    std::string target;
    std::shared_ptr<grpc::ChannelCredentials> creds;
    if (strncmp(this->d_url, HTTP_PREFIX, strlen(HTTP_PREFIX)) == 0) {
        target = this->d_url + strlen(HTTP_PREFIX);
        creds = grpc::InsecureChannelCredentials();
    }
    else if (strncmp(this->d_url, HTTPS_PREFIX, strlen(HTTPS_PREFIX)) == 0) {
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
        target = this->d_url + strlen(HTTPS_PREFIX);
        creds = grpc::SslCredentials(options);
    }
    else if (strncmp(this->d_url, UNIX_SOCKET_PREFIX,
                     strlen(UNIX_SOCKET_PREFIX)) == 0) {
        target = this->d_url;
        creds = grpc::InsecureChannelCredentials();
    }
    else {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Unsupported URL scheme");
    }

    return grpc::CreateChannel(target, creds);
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

    printPadded(padWidth, "--" + p + "retry-limit=INT");
    std::clog << "Number of times to retry on grpc errors\n";

    printPadded(padWidth, "--" + p + "retry-delay=MILLISECONDS");
    std::clog << "How long to wait before the first grpc retry\n";
}

} // namespace buildboxcommon
