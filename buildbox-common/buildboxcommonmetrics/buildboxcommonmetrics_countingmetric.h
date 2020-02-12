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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRIC_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_COUNTINGMETRIC_H

#include <string>

#include <buildboxcommonmetrics_countingmetricvalue.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

/**
 *  CountingMetric
 *
 *  A metric that encapsulates a count (CountingMetricValue).
 */
class CountingMetric {
  private:
    typedef CountingMetricValue ValueType;
    ValueType d_value;
    const std::string d_name;

  public:
    explicit CountingMetric(const std::string &name);
    CountingMetric(const std::string &name, ValueType value);
    void setValue(ValueType value);
    void setValue(ValueType::Count value);
    ValueType value() const;
    void start();
    void stop();
    const std::string &name() const;
    CountingMetric &operator++(int)
    {
        this->d_value++;
        return *this;
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
