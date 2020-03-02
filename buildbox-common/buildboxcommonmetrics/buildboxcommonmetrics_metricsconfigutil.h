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

#ifndef BUILDBOXCOMMONMETRICS_METRICSCONFIGUTIL_H
#define BUILDBOXCOMMONMETRICS_METRICSCONFIGUTIL_H

#include <iostream>

#include <buildboxcommonmetrics_metricsconfigtype.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

class MetricsConfigUtil {
  public:
    // Returns true if the string contains "metric-*"
    static bool isMetricsOption(const std::string &option);

    // Populates the relevant field in config.
    // Argument should be first checked with isMetricOption
    // If the argument doesn't match the hardcoded strings,
    // A RuntimeError is thrown.
    static void metricsParser(const std::string &argument,
                              const std::string &value,
                              MetricsConfigType *config);

    // Given a host:port, store host in serverRet and port in portRet.
    //
    // While parsing the port, this function can throw std::invalid_argument
    // or std::range_error exceptions
    static void parseHostPortString(const std::string &inputString,
                                    std::string *serverRet, uint16_t *portRet);

    // Prints usage strings to std::clog.
    static void usage(std::ostream &out = std::clog);
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif // BUILDBOXCOMMONMETRICS_METRICSCONFIGUTIL_H
