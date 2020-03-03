// Copyright 2019 Bloomberg Finance L.P
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

#include <errno.h>
#include <stdexcept>
#include <string.h>

#include <buildboxcommonmetrics_udpwriter.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

UDPWriter::UDPWriter(int server_port, const std::string &server_name)
    : d_sockfd(-1), d_server_port(std::to_string(server_port)),
      d_server_name(server_name)
{
    // setup hints
    d_hints.ai_family = AF_INET;
    d_hints.ai_socktype = SOCK_DGRAM;
    d_hints.ai_protocol = IPPROTO_UDP;

    connect();
}

void UDPWriter::connect()
{
    struct addrinfo *address_info = nullptr;

    const int return_code = getaddrinfo(
        d_server_name.c_str(), d_server_port.c_str(), &d_hints, &address_info);
    if (return_code != 0) {
        std::string error_msg = "Failed to get address info: " +
                                std::string(gai_strerror(return_code));
        if (return_code == EAI_SYSTEM) {
            error_msg += "\nSystem Error: " + std::string(strerror(errno));
        }
        throw std::runtime_error(error_msg);
    }

    d_sockfd = socket(address_info->ai_family, address_info->ai_socktype,
                      address_info->ai_protocol);
    if (d_sockfd < 0) {
        freeaddrinfo(address_info);
        const std::string error_msg =
            "Could not create UDP Socket to publish metrics: " +
            std::string(strerror(errno));
        throw std::runtime_error(error_msg);
    }

    memcpy(&d_server_address, address_info->ai_addr, address_info->ai_addrlen);
    freeaddrinfo(address_info);
}

void UDPWriter::write(const std::string &buffer)
{
    sendto(d_sockfd, buffer.c_str(), buffer.size(), 0, &d_server_address,
           sizeof(d_server_address));
}

UDPWriter::~UDPWriter()
{
    if (d_sockfd) {
        close(d_sockfd);
    }
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
