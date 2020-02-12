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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRICUTIL_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRICUTIL_H

#include <buildboxcommonmetrics_countingmetric.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

class Collector;
/**
 *  CountingMetricUtil
 *
 *  Small user facing utility to publish a one-off counted metric
 */
struct CountingMetricUtil {
    static void recordCounterMetric(const CountingMetric &metric);
    static void recordCounterMetric(const std::string &name,
                                    const CountingMetricValue &value);
    static void recordCounterMetric(const std::string &name,
                                    const CountingMetricValue::Count &value);
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
