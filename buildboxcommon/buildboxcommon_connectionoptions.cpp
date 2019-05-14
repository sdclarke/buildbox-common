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

static std::string getFileContents(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::string("Failed to open file ") +
                                 filename + std::string(": ") +
                                 std::strerror(errno));
    }

    std::string contents;

    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());

    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    if (!in) {
        in.close();
        throw std::runtime_error(std::string("Failed to read file ") +
                                 filename + std::string(": ") +
                                 std::strerror(errno));
    }

    in.close();
    return contents;
}

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

bool ConnectionOptions::parseArg(const char *arg, const char *prefix)
{
    if (arg == nullptr || arg[0] != '-' || arg[1] != '-') {
        return false;
    }
    arg += 2;
    if (prefix != nullptr) {
        const int prefixLen = strlen(prefix);
        if (strncmp(arg, prefix, prefixLen) != 0) {
            return false;
        }
        arg += prefixLen;
    }
    const char *assign = strchr(arg, '=');
    if (assign) {
        const int keyLen = assign - arg;
        const char *value = assign + 1;
        if (strncmp(arg, "remote", keyLen) == 0) {
            this->d_url = value;
            return true;
        }
        else if (strncmp(arg, "server-cert", keyLen) == 0) {
            this->d_serverCert = value;
            return true;
        }
        else if (strncmp(arg, "client-key", keyLen) == 0) {
            this->d_clientKey = value;
            return true;
        }
        else if (strncmp(arg, "client-cert", keyLen) == 0) {
            this->d_clientCert = value;
            return true;
        }
        else if (strncmp(arg, "retry-limit", keyLen) == 0) {
            this->d_retryLimit = value;
            return true;
        }
        else if (strncmp(arg, "retry-delay", keyLen) == 0) {
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
    if (this->d_serverCert != nullptr) {
        out->push_back("--" + p +
                       "server-cert=" + std::string(this->d_serverCert));
    }
    if (this->d_clientKey != nullptr) {
        out->push_back("--" + p +
                       "client-key=" + std::string(this->d_clientKey));
    }
    if (this->d_clientCert != nullptr) {
        out->push_back("--" + p +
                       "client-cert=" + std::string(this->d_clientCert));
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
            options.pem_root_certs = getFileContents(this->d_serverCert);
        }
        if (this->d_clientKey) {
            options.pem_private_key = getFileContents(this->d_clientKey);
        }
        if (this->d_clientCert) {
            options.pem_cert_chain = getFileContents(this->d_clientCert);
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
        throw std::runtime_error("Unsupported URL scheme");
    }

    return grpc::CreateChannel(target, creds);
}

void ConnectionOptions::printArgHelp(int padWidth, const char *serviceName,
                                     const char *prefix)
{
    std::string p(prefix == nullptr ? "" : prefix);

    printPadded(padWidth, "--" + p + "remote=URL");
    std::clog << "URL for " << serviceName << " service\n";

    printPadded(padWidth, "--" + p + "server-cert=PATH");
    std::clog << "Public server certificate for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-key=PATH");
    std::clog << "Private client key for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-cert=PATH");
    std::clog << "Public client certificate for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "retry-limit=INT");
    std::clog << "Number of times to retry on grpc errors\n";

    printPadded(padWidth, "--" + p + "retry-delay=SECONDS");
    std::clog << "How long to wait between grpc retries\n";
}

} // namespace buildboxcommon
