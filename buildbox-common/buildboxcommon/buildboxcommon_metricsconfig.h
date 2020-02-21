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

#ifndef BUILDBOXCOMMON_METRICSCONFIG_H
#define BUILDBOXCOMMON_METRICSCONFIG_H

#include <iostream>

#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_publisherguard.h>
#include <buildboxcommonmetrics_statsdpublisher.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

// This typedef here specifies the Metric ValueTypes
// we want the publisher to publish
typedef decltype(
    StatsDPublisher<DurationMetricValue, TotalDurationMetricValue>())
    StatsDPublisherType;

class MetricsConfig {
  public:
    explicit MetricsConfig();
    explicit MetricsConfig(const std::string &file,
                           const std::string &udp_server, const bool enable);
    // Given a host:port, store host in serverRet and port in portRet.
    //
    // If metrics are enabled and both output options are specified, this
    // function will throw a runtimerror
    void parseHostPortString(const std::string &inputString,
                             std::string &serverRet, int &portRet);

    // Return a publisher type of either stderr, udp server or file.
    //
    // If both, and metrics are enabled, throw a runtime error.
    // If neither, and metrics are enabled,  defaults metrics publishing to
    // stderr.
    StatsDPublisherType getStatsdPublisherFromConfig();

    // Return true if argument matches one of the predefined strings.
    // False otherwise.
    bool metricsParser(const std::string &argument, const std::string &value);
    void usage();

    // Setters and Getters
    void setFile(const std::string &val);
    const std::string &file() const;

    void setUdpServer(const std::string &val);
    const std::string &udp_server() const;

    void setEnable(const bool val);
    bool enable() const;

  private:
    std::string d_file;
    std::string d_udpServer;
    bool d_enable;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif // BUILDBOXCOMMON_METICSCONFIG_H
