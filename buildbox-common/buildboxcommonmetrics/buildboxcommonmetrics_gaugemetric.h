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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRIC_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_GAUGEMETRIC_H

#include <buildboxcommonmetrics_gaugemetricvalue.h>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

class GaugeMetric final {
  public:
    explicit GaugeMetric(const std::string &name,
                         const GaugeMetricValue &value);

    const std::string &name() const;
    GaugeMetricValue value() const;

  private:
    const std::string d_name;
    const GaugeMetricValue d_value;
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
