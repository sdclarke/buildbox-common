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

#ifndef BUILDBOXCOMMONMETRICS_METRICSCONFIGURATOR_H
#define BUILDBOXCOMMONMETRICS_METRICSCONFIGURATOR_H

#include <iostream>

#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_metricsconfigtype.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

class MetricsConfigurator {
  public:
    // Return a MetricsConfigType type
    // If publishing interval isn't set, defaults to 15 seconds.
    // This value is only used if PeriodicPublisherDaemon is used.
    static MetricsConfigType
    createMetricsConfig(const std::string &file, const std::string &udp_server,
                        const bool enable,
                        const size_t interval =
                            buildboxcommonmetrics::DEFAULT_PUBLISH_INTERVAL);

    // Returns true if the string contains "metric-*"
    static bool isMetricsOption(const std::string &option);

    // Populates the relevant field in config.
    // Argument should be first checked with isMetricOption
    // If the argument doesn't match the hardcoded strings,
    // A RuntimeError is thrown.
    static void metricsParser(const std::string &argument,
                              const std::string &value,
                              MetricsConfigType *config);

    // Prints usage strings to std::clog.
    static void usage(std::ostream &out = std::clog);
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif // BUILDBOXCOMMONMETRICS_METICSCONFIGURATOR_H
