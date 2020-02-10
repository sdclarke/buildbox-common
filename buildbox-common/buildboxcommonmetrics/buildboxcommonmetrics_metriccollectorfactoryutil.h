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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_METRICCOLLECTORFACTORYUTIL_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_METRICCOLLECTORFACTORYUTIL_H

#include <buildboxcommonmetrics_metriccollectorfactory.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

struct MetricCollectorFactoryUtil {
    // The 'MetricCollectorFactoryUtil' component provides a simple wrapper
    // around the 'MetricCollectorFactory' singleton to fetch the singleton and
    // store the specified 'metric' having the specified 'value'.
    template <typename ValueType>
    static void store(const std::string &metric, const ValueType value,
                      MetricCollector<ValueType> *overrideCollector = nullptr)
    {
        MetricCollector<ValueType> *collector =
            overrideCollector
                ? overrideCollector
                : MetricCollectorFactory::getCollector<ValueType>();
        if (MetricCollectorFactory::getInstance()->metricsEnabled()) {
            collector->store(metric, value);
        }
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
