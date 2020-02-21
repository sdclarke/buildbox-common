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

#include <buildboxcommon_exception.h>
#include <buildboxcommon_metricsconfig.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

MetricsConfig::MetricsConfig() : d_file(), d_udpServer(), d_enable(false) {}
MetricsConfig::MetricsConfig(const std::string &file,
                             const std::string &udp_server, const bool enable)
    : d_file(file), d_udpServer(udp_server), d_enable(enable)
{
}

void MetricsConfig::parseHostPortString(const std::string &inputString,
                                        std::string &serverRet, int &portRet)
{
    // NOTE: This only works for IPv4 addresses, not IPv6.
    const std::size_t split_at = inputString.find_last_of(":");
    if (split_at != std::string::npos &&
        split_at + 2 <
            inputString.size()) { // e.g. `localhost:1` or `example.org:1`
        try {
            portRet = std::stoi(inputString.substr(split_at + 1));
            serverRet =
                inputString.substr(0, std::min(split_at, inputString.size()));
        }
        catch (const std::invalid_argument &) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::invalid_argument,
                "Invalid port specified (cannot be parsed to int): '" +
                    inputString.substr(split_at + 1) + "'");
        }
    }
    else { // e.g. `localhost` or `localhost:`
        // default to port 0 if not specified in the string
        portRet = 0;
        serverRet =
            inputString.substr(0, std::min(split_at, inputString.size()));
    }
}

StatsDPublisherType MetricsConfig::getStatsdPublisherFromConfig()
{
    if (d_enable && !d_udpServer.empty() && !d_file.empty()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Error cannot specify both [metrics-udp-server] "
            "and [metrics-file].");
    }

    buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod
        publishMethod = buildboxcommonmetrics::StatsDPublisherOptions::
            PublishMethod::StdErr;

    std::string publishPath = "";
    int publishPort = 0;

    if (!d_udpServer.empty()) {
        publishMethod =
            buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::UDP;
        parseHostPortString(d_udpServer, publishPath, publishPort);
    }
    else if (!d_file.empty()) {
        publishMethod =
            buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::File;
        publishPath = d_file;
    }

    return StatsDPublisherType(publishMethod, publishPath, publishPort);
}

bool MetricsConfig::metricsParser(const std::string &argument_name,
                                  const std::string &value)
{
    if (argument_name == "metrics-enable") {
        d_enable = true;
        return true;
    }
    else if (argument_name == "metrics-file") {
        d_file = value;
        d_enable = true;
        return true;
    }
    else if (argument_name == "metrics-udp-server") {
        d_udpServer = value;
        d_enable = true;
        return true;
    }
    else {
        return false;
    }
}

void MetricsConfig::usage()
{
    std::clog << "    --metrics-enable            Enable metric "
                 "collection (Defaults to False)"
              << std::endl;
    std::clog << "    --metrics-file              Write metrics to that file "
                 "(Default/Empty string — "
                 "(stderr).\n               Cannot be used with "
                 "--metrics-udp-server."
              << std::endl;
    std::clog << "    --metrics-udp-server        Write metrics to the "
                 "specified host:UDP_PORT\n"
                 "                                Cannot be used with "
                 "--metrics-file"
              << std::endl;
}

void MetricsConfig::setFile(const std::string &val) { d_file = val; }
const std::string &MetricsConfig::file() const { return d_file; }

void MetricsConfig::setUdpServer(const std::string &val) { d_udpServer = val; }
const std::string &MetricsConfig::udp_server() const { return d_udpServer; }

void MetricsConfig::setEnable(const bool val) { d_enable = val; }
bool MetricsConfig::enable() const { return d_enable; }

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
