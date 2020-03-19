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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_METRICCOLLECTOR_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_METRICCOLLECTOR_H

#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/**
 * MetricCollector
 *
 * Collects the values of Metrics of type ValueType by name
 * and aggregates them if ValueType isAggregatable.
 *
 * This class is using SFINAE to make it so that different behavior
 * can be implemented for different kinds of ValueTypes.
 * (e.g. aggregatable/non-aggregatable)
 *
 * SFINAE ref: https://en.cppreference.com/w/cpp/language/sfinae
 */
template <class ValueType, typename = void> class MetricCollector;

// Template class for Aggregatable Metrics
template <class ValueType>
class MetricCollector<ValueType, std::enable_if_t<ValueType::isAggregatable>> {
  private:
    typedef std::unordered_map<std::string, ValueType> MetricsMap;
    MetricsMap d_metrics;
    std::mutex d_metrics_mutex;

  public:
    // Constructor
    MetricCollector() = default;

    // Destructor
    ~MetricCollector() = default;

    // Store this in our internal map...
    void store(const std::string &name, const ValueType value)
    {
        // For aggregatable entries, the associated map in `d_metrics`, will
        // contain at most one entry for each metric name.
        // With subsequent calls to store() for that metric_name, the
        // newly-collected input will be aggregated using `operator++`.

        std::lock_guard<std::mutex> lock(d_metrics_mutex);

        auto entry = d_metrics.find(name);
        if (entry != d_metrics.end()) {
            entry->second += value;
        }
        else {
            d_metrics.emplace(name, value);
        }
    }

    MetricsMap getSnapshot()
    {
        MetricsMap snapshot;
        {
            std::lock_guard<std::mutex> lock(d_metrics_mutex);
            d_metrics.swap(snapshot);
        }
        return snapshot;
    }
};

// Template class for NON-Aggregatable Metrics
template <class ValueType>
class MetricCollector<ValueType,
                      std::enable_if_t<!ValueType::isAggregatable>> {
  private:
    typedef std::pair<std::string, ValueType> MetricPairType;
    typedef std::vector<MetricPairType> MetricsContainer;
    MetricsContainer d_metrics;
    std::mutex d_metrics_mutex;

  public:
    // Constructor
    MetricCollector() = default;

    // Destructor
    ~MetricCollector() = default;

    // Store this in our internal container...
    void store(const std::string &name, const ValueType value)
    {
        // Since this type is non-aggregatable, `d_metrics` will keep all
        // values in the same order in which they are collected, in a vector.
        std::lock_guard<std::mutex> lock(d_metrics_mutex);
        d_metrics.emplace(d_metrics.end(), MetricPairType(name, value));
    }

    MetricsContainer getSnapshot()
    {
        MetricsContainer snapshot;
        {
            std::lock_guard<std::mutex> lock(d_metrics_mutex);
            d_metrics.swap(snapshot);
        }
        return snapshot;
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
