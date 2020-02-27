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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRICUTIL_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRICUTIL_H

#include <buildboxcommonmetrics_gaugemetric.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {
/*
 * Public interface to publish gauge metrics.
 */
struct GaugeMetricUtil {
    // Publishes the `GaugeMetric` object
    static void recordGauge(const GaugeMetric &metric);

    // Publishes a gauge entry with an absolute value
    static void
    setGauge(const std::string &name,
             const GaugeMetricValue::GaugeMetricNumericType &value);

    // Publishes a gauge entry with a relative value
    static void
    adjustGauge(const std::string &name,
                const GaugeMetricValue::GaugeMetricNumericType &delta);
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
