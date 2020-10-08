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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_DISTRIBUTIONMETRICVALUE_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_DISTRIBUTIONMETRICVALUE_H

#include <cstdint>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

class DistributionMetricValue final {
  public:
    typedef int64_t DistributionMetricNumericType;

    explicit DistributionMetricValue(DistributionMetricNumericType value);

    DistributionMetricNumericType value() const;

    const std::string toStatsD(const std::string &name) const;

    bool operator==(const DistributionMetricValue &other) const;

    bool operator!=(const DistributionMetricValue &other) const;

    // This metric value is not aggregatable (aggregations are performed
    // server-side):
    static const bool isAggregatable = false;
    DistributionMetricValue &
    operator+=(const DistributionMetricValue &other) = delete;

  private:
    DistributionMetricNumericType d_value;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
