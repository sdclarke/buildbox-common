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

#include <limits>

#include <buildboxcommonmetrics_metricsconfigutil.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

bool MetricsConfigUtil::isMetricsOption(const std::string &option)
{
    return bool(option.find("metrics-") == 0);
}

void MetricsConfigUtil::metricsParser(const std::string &argument_name,
                                      const std::string &value,
                                      MetricsConfigType *config)
{

    if (argument_name == "metrics-mode") {
        if (value == "stderr" || value == "stderr://") {
            config->setEnable(true);
            // No need to set anything as this is the default behaviour for the
            // publishers.
            return;
        }

        const std::string separator = "://";
        auto separator_pos = value.find(separator);
        if (separator_pos == std::string::npos) {
            throw std::runtime_error("Metric value format incorrect: [" +
                                     value + "] for input: [" + argument_name +
                                     "]. See --help.");
        }

        auto type = value.substr(0, separator_pos);
        auto result = value.substr(separator_pos + separator.length());

        if (result.empty()) {
            throw std::runtime_error(
                "Incorrect metrics output option value: [" + result +
                "] parsed from input: [" + argument_name + "]. See --help.");
        }

        if (type == "udp") {
            config->setEnable(true);
            config->setUdpServer(result);
        }
        else if (type == "file") {
            config->setEnable(true);
            config->setFile(result);
        }
        else {
            throw std::runtime_error("Unknown metrics output option type: [" +
                                     type + "] parsed from input: [" +
                                     argument_name + "]. See --help.");
        }
    }
    else if (argument_name == "metrics-publish-interval") {
        config->setInterval(std::stoi(value));
    }
    else {
        throw std::runtime_error("Unknown metrics option: [" + argument_name +
                                 "]. See --help.");
    }
    return;
}

void MetricsConfigUtil::usage(std::ostream &out)
{
    out << "    --metrics-mode=MODE   Options for MODE are:\n"
           "           udp://localhost:50051\n"
           "           file:///tmp\n"
           "           stderr\n"
           "                          Only one metric output "
           "mode can be specified"
        << std::endl;
    out << "    --metrics-publish-interval=VALUE   Publish metric at the "
           "specified interval rate in seconds, defaults "
        << buildboxcommonmetrics::DEFAULT_PUBLISH_INTERVAL << " seconds"
        << std::endl;
}

void MetricsConfigUtil::parseHostPortString(const std::string &inputString,
                                            std::string *serverRet,
                                            uint16_t *portRet)
{
    if (serverRet == nullptr || portRet == nullptr) {
        throw std::invalid_argument(
            "null pointer detected in output parameters");
    }

    // NOTE: This only works for IPv4 addresses, not IPv6.
    const std::size_t split_at = inputString.find_last_of(":");
    if (split_at != std::string::npos &&
        split_at + 2 <
            inputString.size()) { // e.g. `localhost:1` or `example.org:1`
        try {
            const uint32_t tmp =
                (uint32_t)std::stoul(inputString.substr(split_at + 1));
            if (tmp > std::numeric_limits<uint16_t>::max()) {
                throw std::range_error(
                    "Invalid port specified (value too large): '" +
                    inputString.substr(split_at + 1) + "'");
            }
            *portRet = (uint16_t)tmp;
            *serverRet =
                inputString.substr(0, std::min(split_at, inputString.size()));
        }
        catch (const std::invalid_argument &) {
            throw std::invalid_argument(
                "Invalid port specified (cannot be parsed to int): '" +
                inputString.substr(split_at + 1) + "'");
        }
    }
    else { // e.g. `localhost` or `localhost:`
        // default to port 8125 if not specified in the string
        *portRet = 8125;
        *serverRet =
            inputString.substr(0, std::min(split_at, inputString.size()));
    }
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
