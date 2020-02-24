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

#include <buildboxcommonmetrics_metricsconfigtype.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

MetricsConfigType::MetricsConfigType(const std::string &file,
                                     const std::string &udp_server,
                                     const bool enable,
                                     const size_t publish_interval)
    : d_file(file), d_udpServer(udp_server), d_enable(enable),
      d_publishInterval(publish_interval)
{
}

MetricsConfigType::MetricsConfigType()
    : d_file(), d_udpServer(), d_enable(false),
      d_publishInterval(DEFAULT_PUBLISH_INTERVAL)
{
}

void MetricsConfigType::setFile(const std::string &val) { d_file = val; }
const std::string &MetricsConfigType::file() const { return d_file; }

void MetricsConfigType::setUdpServer(const std::string &val)
{
    d_udpServer = val;
}
const std::string &MetricsConfigType::udp_server() const
{
    return d_udpServer;
}

void MetricsConfigType::setEnable(const bool val) { d_enable = val; }
bool MetricsConfigType::enable() const { return d_enable; }

void MetricsConfigType::setInterval(const size_t val)
{
    d_publishInterval = val;
}
size_t MetricsConfigType::interval() const { return d_publishInterval; }

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
