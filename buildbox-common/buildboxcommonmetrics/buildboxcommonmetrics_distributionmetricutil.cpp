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

#include <buildboxcommonmetrics_distributionmetricutil.h>

#include <buildboxcommonmetrics_metriccollectorfactoryutil.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

void DistributionMetricUtil::recordDistributionMetric(
    const DistributionMetric &metric)
{
    MetricCollectorFactoryUtil::store(metric.name(), metric.value());
}

void DistributionMetricUtil::recordDistributionMetric(
    const std::string &name,
    const DistributionMetricValue::DistributionMetricNumericType &value)
{
    recordDistributionMetric(
        DistributionMetric(name, DistributionMetricValue(value)));
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
