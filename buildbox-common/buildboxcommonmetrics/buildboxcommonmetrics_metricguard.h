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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_METRICGUARD_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_METRICGUARD_H

#include <string>

#include <buildboxcommonmetrics_scopedmetric.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

template <class ValueType> class MetricCollector;

/**
 * MetricGuard is the Metric Guard class
 *
 * It invokes start() and stop() on MetricType upon creation and destruction
 * of the MetricGuard and forwards the value of the metric ValueType to the
 * appropriate collector (provided by the MetricCollectorFactory)
 */
template <class MetricType> class MetricGuard {
  private:
    // Infer the type of the value of MetricType
    // by inspecting the type MetricType.value() returns
    MetricType d_metric;
    typedef typename ScopedMetric<MetricType>::ValueType ValueType;
    ScopedMetric<MetricType> d_scopedMetric;

  public:
    explicit MetricGuard(const std::string &name,
                         MetricCollector<ValueType> *collector = nullptr)
        : d_metric(name), d_scopedMetric(&d_metric, collector)
    {
    }

    // DEPRECATED: the boolean flag indicating whether this metric is
    // enabled/disabled is no longer respected and will be removed in a later
    // version. The enablement of metrics is now configured at the
    // MetricCollectorFactory level (since it should be global, not per
    // MetricGuard instance).
    MetricGuard(const std::string &name, bool,
                MetricCollector<ValueType> *collector = nullptr)
        : MetricGuard(name, collector)
    {
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
