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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRICVALUES_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRICVALUES_H

#include <cstdint>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/*
 * A StatsD gauge payload can be of two types: an absolute value
 * ("set this gauge to 256"), or a relative one ("increment this gauge 2
 * units").
 */
class GaugeMetricValue final {
  public:
    typedef int64_t GaugeMetricNumericType;

    explicit GaugeMetricValue(GaugeMetricNumericType value,
                              bool is_delta = false);

    GaugeMetricNumericType value() const;

    bool isDelta() const;

    const std::string toStatsD(const std::string &name) const;

    // We allow the `MetricCollector` to aggregate entries. That is, combining
    // two `GaugeMetricValue` instances into one.
    static const bool isAggregatable = true;

    // The collector will use the `+=` operator to aggregate them, so this
    // implements the combination logic. (Note that in some cases it will
    // overwrite the left-hand side with the right.)
    GaugeMetricValue &operator+=(const GaugeMetricValue &other);

    bool operator==(const GaugeMetricValue &other) const;

    bool operator!=(const GaugeMetricValue &other) const;

  private:
    GaugeMetricNumericType d_value;
    bool d_is_delta;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
