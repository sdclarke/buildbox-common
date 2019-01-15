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

#include <fstream>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <stdexcept>

namespace BloombergLP {
namespace buildboxcommon {

namespace {
static std::string getFileContents(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::string("Failed to open file ") +
                                 filename);
    }

    std::string contents;

    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());

    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    if (!in) {
        in.close();
        throw std::runtime_error(std::string("Failed to read file ") +
                                 filename);
    }

    in.close();
    return contents;
}

static void printPadded(int padWidth, const std::string &str)
{
    std::cerr << "    " << str;
    for (int i = str.length() + 4; i < padWidth; ++i) {
        std::cerr << " ";
    }
}
} // namespace

bool ConnectionOptions::parseArg(const char *arg, const char *prefix)
{
    if (arg == nullptr || arg[0] != '-' || arg[1] != '-') {
        return false;
    }
    arg += 2;
    if (prefix != nullptr) {
        int prefixLen = strlen(prefix);
        if (strncmp(arg, prefix, prefixLen) != 0) {
            return false;
        }
        arg += prefixLen;
    }
    const char *assign = strchr(arg, '=');
    if (assign) {
        int keyLen = assign - arg;
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
    }
    return false;
}

void ConnectionOptions::putArgs(std::vector<std::string> *out,
                                const char *prefix) const
{
    std::string p(prefix == nullptr ? "" : prefix);
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
}

std::shared_ptr<grpc::Channel> ConnectionOptions::createChannel() const
{
    std::string target;
    std::shared_ptr<grpc::ChannelCredentials> creds;
    if (strncmp(this->d_url, "http://", strlen("http://")) == 0) {
        target = this->d_url + strlen("http://");
        creds = grpc::InsecureChannelCredentials();
    }
    else if (strncmp(this->d_url, "https://", strlen("https://")) == 0) {
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
        target = this->d_url + strlen("https://");
        creds = grpc::SslCredentials(options);
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
    std::cerr << "URL for " << serviceName << " service\n";

    printPadded(padWidth, "--" + p + "server-cert=PATH");
    std::cerr << "Public server certificate for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-key=PATH");
    std::cerr << "Private client key for TLS (PEM-encoded)\n";

    printPadded(padWidth, "--" + p + "client-cert=PATH");
    std::cerr << "Public client certificate for TLS (PEM-encoded)\n";
}

} // namespace buildboxcommon
} // namespace BloombergLP
