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

#ifndef BUILDBOXCOMMONMETRICS_STATSDPUBLISHERCREATOR_H
#define BUILDBOXCOMMONMETRICS_STATSDPUBLISHERCREATOR_H

#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_metricsconfigtype.h>
#include <buildboxcommonmetrics_publisherguard.h>
#include <buildboxcommonmetrics_statsdpublisher.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>
#include <memory.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

// This typedef here specifies the Metric ValueTypes
// we want the publisher to publish
typedef decltype(
    StatsDPublisher<DurationMetricValue, TotalDurationMetricValue>())
    StatsDPublisherType;

class StatsdPublisherCreator {
  public:
    // Return a publisher type of either stderr, udp server or file.
    //
    // If 'metricsConfig' specifies a udpServer, the parsing of the port
    // may throw std::invalid_argument or std::range_error exceptions
    static std::shared_ptr<StatsDPublisherType>
    createStatsdPublisher(const MetricsConfigType &metricsConfig);

    // Given a host:port, store host in serverRet and port in portRet.
    //
    // While parsing the port, this function can throw std::invalid_argument
    // or std::range_error exceptions
    static void parseHostPortString(const std::string &inputString,
                                    std::string *serverRet, uint16_t *portRet);

    StatsdPublisherCreator() = delete;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
