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

#include <stdexcept>

#include <buildboxcommonmetrics_metricsconfigurator.h>

#include <buildboxcommonmetrics_metricsconfigutil.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

MetricsConfigType MetricsConfigurator::createMetricsConfig(
    const std::string &file, const std::string &udp_server, const bool enable,
    const size_t interval)
{
    if (enable && !udp_server.empty() && !file.empty()) {
        throw std::runtime_error(
            "Error cannot specify both [metrics-udp-server] "
            "and [metrics-file].");
    }

    return MetricsConfigType(file, udp_server, enable, interval);
}

// DEPRECATED functions aliased to MetricsConfigUtil
bool MetricsConfigurator::isMetricsOption(const std::string &option)
{
    return MetricsConfigUtil::isMetricsOption(option);
}

void MetricsConfigurator::metricsParser(const std::string &argument_name,
                                        const std::string &value,
                                        MetricsConfigType *config)
{
    MetricsConfigUtil::metricsParser(argument_name, value, config);
}

void MetricsConfigurator::usage(std::ostream &out)
{
    MetricsConfigUtil::usage(out);
}

void MetricsConfigurator::parseHostPortString(const std::string &inputString,
                                              std::string *serverRet,
                                              uint16_t *portRet)
{
    MetricsConfigUtil::parseHostPortString(inputString, serverRet, portRet);
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
