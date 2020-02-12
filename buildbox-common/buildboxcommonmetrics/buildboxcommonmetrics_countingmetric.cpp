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

#include <buildboxcommonmetrics_countingmetric.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

CountingMetric::CountingMetric(const std::string &name)
    : CountingMetric(name, ValueType(0))
{
}

CountingMetric::CountingMetric(const std::string &name, ValueType value)
    : d_value(value), d_name(name)
{
}
void CountingMetric::setValue(ValueType value) { d_value = value; }

void CountingMetric::setValue(ValueType::Count value)
{
    d_value = ValueType(value);
}
void CountingMetric::start() { d_value++; }

void CountingMetric::stop()
{
    // Nothing to do here
}

CountingMetric::ValueType CountingMetric::value() const { return d_value; }

const std::string &CountingMetric::name() const { return d_name; }

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
