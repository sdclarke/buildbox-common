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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_DISTRIBUTIONMETRIC_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_DISTRIBUTIONMETRIC_H

#include <buildboxcommonmetrics_distributionmetricvalue.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

class DistributionMetric final {
    /*
     * This metric type allows to represent the statistical distribution of a
     * set of values (potentially from distributed sources) during a time
     * interval.
     */
  public:
    explicit DistributionMetric(const std::string &name,
                                const DistributionMetricValue &value);

    const std::string &name() const;
    DistributionMetricValue value() const;

  private:
    const std::string d_name;
    const DistributionMetricValue d_value;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
