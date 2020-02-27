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

#include <buildboxcommonmetrics_gaugemetricutil.h>

#include <buildboxcommonmetrics_gaugemetric.h>
#include <buildboxcommonmetrics_metriccollectorfactoryutil.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

void GaugeMetricUtil::recordGauge(const GaugeMetric &metric)
{
    MetricCollectorFactoryUtil::store(metric.name(), metric.value());
}

void buildboxcommon::buildboxcommonmetrics::GaugeMetricUtil::setGauge(
    const std::string &name,
    const GaugeMetricValue::GaugeMetricNumericType &value)
{
    recordGauge(GaugeMetric(name, GaugeMetricValue(value, false)));
}

void GaugeMetricUtil::adjustGauge(
    const std::string &name,
    const GaugeMetricValue::GaugeMetricNumericType &delta)
{
    recordGauge(GaugeMetric(name, GaugeMetricValue(delta, true)));
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
