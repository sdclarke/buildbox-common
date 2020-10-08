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

#include <buildboxcommonmetrics_distributionmetricvalue.h>

#include <sstream>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

DistributionMetricValue::DistributionMetricValue(
    DistributionMetricValue::DistributionMetricNumericType value)
    : d_value(value)
{
}

DistributionMetricValue::DistributionMetricNumericType
DistributionMetricValue::value() const
{
    return d_value;
}

const std::string
DistributionMetricValue::toStatsD(const std::string &name) const
{
    std::ostringstream os;
    os << name << ":" << value() << "|d";
    return os.str();
}

bool DistributionMetricValue::
operator==(const DistributionMetricValue &other) const
{
    return this->value() == other.value();
}

bool DistributionMetricValue::
operator!=(const DistributionMetricValue &other) const
{
    return !(*this == other);
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
