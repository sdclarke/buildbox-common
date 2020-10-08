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

#include <buildboxcommonmetrics_distributionmetric.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

DistributionMetric::DistributionMetric(const std::string &name,
                                       const DistributionMetricValue &value)
    : d_name(name), d_value(value)
{
}

const std::string &DistributionMetric::name() const { return d_name; }

DistributionMetricValue DistributionMetric::value() const { return d_value; }

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
