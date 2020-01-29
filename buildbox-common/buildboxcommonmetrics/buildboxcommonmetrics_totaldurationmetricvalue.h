// Copyright 2019 Bloomberg Finance L.P
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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_TOTALDURATIONMETRICVALUE_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_TOTALDURATIONMETRICVALUE_H

#include <chrono>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/**
 * TotalDurationMetricValue
 */
class TotalDurationMetricValue {
  public:
    // Make it easy to switch this out to any time denomination  (e.g.
    // ms/us/ns/etc)
    typedef std::chrono::microseconds TimeDenomination;

    static const bool isAggregatable = true;

    void setValue(TimeDenomination value);
    TimeDenomination value() const;

    explicit TotalDurationMetricValue(
        TimeDenomination value = TimeDenomination(0));

    TotalDurationMetricValue &
    operator+=(const TotalDurationMetricValue &other);

    const std::string toStatsD(const std::string &myName) const;

  private:
    TimeDenomination d_value;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
