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
#include <buildboxcommonmetrics_metricsconfigurator.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/**
 * Helper methods used to implement `clearAllMetricCollection`.
 * Should not be used outside this file.
 */
struct InternalTemplatedMethods {

    /**
     * Tuple containing all publisher value types.
     */
    using PublishedValueTypesTuple =
        MetricsConfigurator::publisherTypeOfAllValueTypes<std::tuple>;

    /**
     * Base case for templated function call.
     * Once the index of the tuple exceeds or equals its size,
     * this method will be called to terminate the recursive call.
     */
    template <std::size_t index,
              typename std::enable_if<(
                  index >= std::tuple_size<PublishedValueTypesTuple>::value)>::
                  type * = nullptr>
    static void clearAllMetricValueTypes()
    {
    }

    /**
     * Recursive templated function.
     * Will clear container for metric value type associated with index.
     * Will then recurse, calling method for index +1.
     */
    template <std::size_t index,
              typename std::enable_if<
                  (index < std::tuple_size<PublishedValueTypesTuple>::value)>::
                  type * = nullptr>
    static void clearAllMetricValueTypes()
    {
        const auto collector =
            MetricCollectorFactory::getCollector<typename std::tuple_element<
                index, PublishedValueTypesTuple>::type>();
        collector->getSnapshot();

        clearAllMetricValueTypes<index + 1>();
    }
};

template <typename MetricType>
bool validateMetricCollection(const std::string &metric)
{
    typedef decltype(std::declval<MetricType>().value()) ValueType;
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_container = collector->getSnapshot();
    return std::count_if(
        metrics_container.begin(), metrics_container.end(),
        [&metric](
            const std::pair<std::string, ValueType> &metric_key_value_pair) {
            return metric_key_value_pair.first == metric;
        });
}

template <typename ValueType>
bool validateMetricCollection(const std::string &name, const ValueType &value)
{
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_container = collector->getSnapshot();
    const auto entry_iterator = std::find_if(
        metrics_container.begin(), metrics_container.end(),
        [&name, &value](
            const std::pair<std::string, ValueType> &metric_key_value_pair) {
            return metric_key_value_pair.first == name &&
                   metric_key_value_pair.second == value;
        });

    return entry_iterator != metrics_container.cend() &&
           entry_iterator->second == value;
}

template <typename MetricType>
bool validateMetricCollection(const std::vector<std::string> &metrics)
{
    typedef decltype(std::declval<MetricType>().value()) ValueType;
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    const auto metrics_container = collector->getSnapshot();
    for (const auto &metric : metrics) {
        const auto entry_iterator =
            std::find_if(metrics_container.begin(), metrics_container.end(),
                         [&metric](const std::pair<std::string, ValueType>
                                       &metric_key_value_pair) {
                             return metric_key_value_pair.first == metric;
                         });
        if (entry_iterator == metrics_container.cend()) {
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
    const auto metrics_container = collector->getSnapshot();
    for (const auto &metric : name_values) {
        const auto entry_iterator = std::find_if(
            metrics_container.begin(), metrics_container.end(),
            [&metric](const std::pair<std::string, ValueType>
                          &metric_key_value_pair) {
                return metric_key_value_pair.first == metric.first &&
                       metric_key_value_pair.second == metric.second;
            });
        if (entry_iterator == metrics_container.cend()) {
            return false;
        }
        else if (entry_iterator->second != metric.second) {
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
    const auto metrics_container = collector->getSnapshot();
    // First verify that none of the expected missing metrics are there.
    for (const std::string &metric_name : expectedMissingMetrics) {
        if (std::count_if(
                metrics_container.begin(), metrics_container.end(),
                [&metric_name](const std::pair<std::string, ValueType>
                                   &metric_key_value_pair) {
                    return metric_key_value_pair.first == metric_name;
                })) {
            return false;
        }
    }

    for (const auto &metric : expectedMetrics) {
        const auto entry_iterator = std::find_if(
            metrics_container.begin(), metrics_container.end(),
            [&metric](const std::pair<std::string, ValueType>
                          &metric_key_value_pair) {
                return metric_key_value_pair.first == metric.first &&
                       metric_key_value_pair.second == metric.second;
            });
        if (entry_iterator == metrics_container.cend()) {
            return false;
        }
    }
    return true;
}

template <typename ValueType> void clearMetricCollection()
{
    MetricCollector<ValueType> *collector =
        MetricCollectorFactory::getCollector<ValueType>();
    collector->getSnapshot();
}

void clearAllMetricCollection()
{
    InternalTemplatedMethods::clearAllMetricValueTypes<0>();
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
