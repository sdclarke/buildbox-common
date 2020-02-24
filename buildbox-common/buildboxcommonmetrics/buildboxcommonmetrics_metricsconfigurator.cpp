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
#include <buildboxcommonmetrics_metricsconfigurator.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

MetricsConfigType MetricsConfigurator::createMetricsConfig(
    const std::string &file, const std::string &udp_server, const bool enable,
    const size_t interval)
{
    if (enable && !udp_server.empty() && !file.empty()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Error cannot specify both [metrics-udp-server] "
            "and [metrics-file].");
    }

    return MetricsConfigType(file, udp_server, enable, interval);
}

bool MetricsConfigurator::isMetricsOption(const std::string &option)
{
    return bool(option.find("metrics-") == 0);
}

void MetricsConfigurator::metricsParser(const std::string &argument_name,
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
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Metric value format incorrect: ["
                                        << value << "] for input: ["
                                        << argument_name << "]. See --help.");
        }

        auto type = value.substr(0, separator_pos);
        auto result = value.substr(separator_pos + separator.length());

        if (result.empty()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Incorrect metrics output option value: ["
                                        << result << "] parsed from input: ["
                                        << argument_name << "]. See --help.");
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
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Unknown metrics output option type: ["
                                        << type << "] parsed from input: ["
                                        << argument_name << "]. See --help.");
        }
    }
    else if (argument_name == "metrics-publish-interval") {
        config->setInterval(std::stoi(value));
    }
    else {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Unknown metrics option: [" << argument_name << "]. See --help.");
    }
    return;
}

void MetricsConfigurator::usage(std::ostream &out)
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

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
