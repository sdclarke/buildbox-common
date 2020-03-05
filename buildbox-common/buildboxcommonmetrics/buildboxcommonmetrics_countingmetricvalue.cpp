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

#include <buildboxcommonmetrics_countingmetricvalue.h>

#include <sstream>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

CountingMetricValue::CountingMetricValue(Count value) : d_value(value) {}

void CountingMetricValue::setValue(Count value) { d_value = value; }
CountingMetricValue::Count CountingMetricValue::value() const
{
    return d_value;
}
const std::string
CountingMetricValue::toStatsD(const std::string &myName) const
{
    std::ostringstream os;
    os << myName << ":" << value() << "|c";
    return os.str();
}

CountingMetricValue &CountingMetricValue::
operator+=(const CountingMetricValue &other)
{
    this->d_value += other.value();
    return *this;
}

CountingMetricValue &CountingMetricValue::operator++(int)
{
    ++this->d_value;
    return *this;
}

CountingMetricValue &CountingMetricValue::operator+(CountingMetricValue value)
{
    this->d_value += value.value();
    return *this;
}

CountingMetricValue &CountingMetricValue::operator+=(const Count value)
{
    this->d_value += value;
    return *this;
}

CountingMetricValue &CountingMetricValue::operator+(const Count value)
{
    this->d_value += value;
    return *this;
}

bool CountingMetricValue::operator==(const CountingMetricValue &other) const
{
    return this->value() == other.value();
}

bool CountingMetricValue::operator!=(const CountingMetricValue &other) const
{
    return !(*this == other);
}
} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
