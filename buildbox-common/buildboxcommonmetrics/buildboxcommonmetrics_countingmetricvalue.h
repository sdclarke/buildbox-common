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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRICVALUE_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRICVALUE_H

#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/**
 * CountingMetricValue
 */
class CountingMetricValue {
  public:
    // CLASS DATA
    static const bool isAggregatable = true;

  public:
    // TYPES
    typedef int Count;

    // CREATORS
    explicit CountingMetricValue(Count value = 0);

    // CLASS METHODS
    void setValue(Count value);
    Count value() const;
    const std::string toStatsD(const std::string &myName) const;

    // OPERATORS
    CountingMetricValue &operator+=(const CountingMetricValue &other);
    CountingMetricValue &operator+=(const Count other);
    CountingMetricValue &operator++(int);
    CountingMetricValue &operator+(CountingMetricValue other);
    CountingMetricValue &operator+(Count other);
    bool operator==(const CountingMetricValue &other) const;
    bool operator!=(const CountingMetricValue &other) const;

  private:
    // DATA MEMBERS
    Count d_value;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
