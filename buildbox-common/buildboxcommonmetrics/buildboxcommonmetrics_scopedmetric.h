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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_SCOPEDMETRIC_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_SCOPEDMETRIC_H

#include <buildboxcommonmetrics_metriccollectorfactoryutil.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

template <class ValueType> class MetricCollector;

template <class MetricType> class ScopedMetric {
  public:
    // TYPES
    typedef decltype(std::declval<MetricType>().value()) ValueType;

  private:
    // DATA
    MetricCollector<ValueType> *d_collector;
    MetricType *d_metric;

  public:
    // CREATORS
    explicit ScopedMetric(MetricType *metric,
                          MetricCollector<ValueType> *collector = nullptr)
        : d_collector(collector), d_metric(metric)
    {
        if (MetricCollectorFactory::getInstance()->metricsEnabled()) {
            d_metric->start();
        }
    }

    ~ScopedMetric<MetricType>()
    {
        if (MetricCollectorFactory::getInstance()->metricsEnabled()) {
            d_metric->stop();
            MetricCollectorFactoryUtil::store(d_metric->name(),
                                              d_metric->value(), d_collector);
        }
    };
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
