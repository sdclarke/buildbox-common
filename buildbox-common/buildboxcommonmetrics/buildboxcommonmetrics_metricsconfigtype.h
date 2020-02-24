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

#ifndef BUILDBOXCOMMONMETRICS_METRICSCONFIGTYPE_H
#define BUILDBOXCOMMONMETRICS_METRICSCONFIGTYPE_H

#include <string>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

// Interval used by the periodic publisher.
const size_t DEFAULT_PUBLISH_INTERVAL = 15;

class MetricsConfigType final {
  public:
    explicit MetricsConfigType(
        const std::string &file, const std::string &udp_server,
        const bool enable,
        const size_t publish_interval = DEFAULT_PUBLISH_INTERVAL);

    explicit MetricsConfigType();

    // Setters and Getters
    void setFile(const std::string &val);
    const std::string &file() const;

    void setUdpServer(const std::string &val);
    const std::string &udp_server() const;

    void setEnable(const bool val);
    bool enable() const;

    void setInterval(const size_t val);
    size_t interval() const;

  private:
    std::string d_file;
    std::string d_udpServer;
    bool d_enable;
    size_t d_publishInterval;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif // BUILDBOXCOMMONMETRICS_METICSCONFIG_H
