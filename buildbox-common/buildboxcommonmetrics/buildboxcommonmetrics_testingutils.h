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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_TESTINGUTILS_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_TESTINGUTILS_H

#include <buildboxcommonmetrics_metriccollectorfactory.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

template <typename MetricType>
bool validateMetricCollection(const std::string &metric)
{
    typedef decltype(std::declval<MetricType>().value()) ValueType;
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    auto metrics_map = collector->getSnapshot();
    return metrics_map.count(metric);
}

template <typename ValueType>
bool validateMetricCollection(const std::string &name, const ValueType &value)
{
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_map = collector->getSnapshot();
    const auto entry = metrics_map.find(name);

    return entry != metrics_map.cend() && entry->second == value;
}

template <typename MetricType>
bool validateMetricCollection(const std::vector<std::string> &metrics)
{
    typedef decltype(std::declval<MetricType>().value()) ValueType;
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_map = collector->getSnapshot();
    for (const auto &metric : metrics) {
        if (metrics_map.find(metric) == metrics_map.end()) {
            return false;
        }
    }
    return true;
}

template <typename ValueType>
bool validateMetricCollection(
    const std::vector<std::pair<std::string, ValueType>> &name_values)
{
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_map = collector->getSnapshot();
    for (const auto &metric : name_values) {
        const auto entry = metrics_map.find(metric.first);
        if (entry == metrics_map.cend()) {
            return false;
        }
        else if (entry->second != metric.second) {
            return false;
        }
    }
    return true;
}

template <typename ValueType>
bool validateMetricCollection(
    const std::vector<std::pair<std::string, ValueType>> &expectedMetrics,
    const std::vector<std::string> &expectedMissingMetrics)
{
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_map = collector->getSnapshot();
    // First verify that none of the expected missing metrics are there.
    for (const std::string &metric : expectedMissingMetrics) {
        if (metrics_map.count(metric)) {
            return false;
        }
    }

    for (const auto &metric : expectedMetrics) {
        const auto entry = metrics_map.find(metric.first);
        if (entry == metrics_map.cend()) {
            return false;
        }
        else if (entry->second != metric.second) {
            return false;
        }
    }
    return true;
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
