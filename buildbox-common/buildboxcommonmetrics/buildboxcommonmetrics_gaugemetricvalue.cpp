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

#include <buildboxcommonmetrics_gaugemetricvalue.h>

#include <sstream>
#include <string>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

GaugeMetricValue::GaugeMetricValue(
    GaugeMetricValue::GaugeMetricNumericType value, bool is_delta)
    : d_value(value), d_is_delta(is_delta)
{
}

GaugeMetricValue::GaugeMetricNumericType GaugeMetricValue::value() const
{
    return d_value;
}

bool GaugeMetricValue::isDelta() const { return d_is_delta; }

const std::string GaugeMetricValue::toStatsD(const std::string &name) const
{
    std::ostringstream os;
    os << name << ":";

    if (d_is_delta) {
        const auto sign = (this->value() >= 0) ? "+" : "-";
        os << sign << std::abs(this->value());
    }
    else {
        if (d_value < 0) {
            // Due to the publishing format of the statsd gauges, negative
            // absolute values are not allowed. (The presence of a '+' or '-'
            // is used to indicate that the entry is a delta). Therefore, to
            // set a gauge to a negative value it must first be set to 0.
            os << "0|g\n";
            os << name << ":";
        }
        os << this->value();
    }

    os << "|g";
    return os.str();
}

GaugeMetricValue &GaugeMetricValue::operator+=(const GaugeMetricValue &other)
{
    if (other.isDelta()) { // We can sum the delta to our value
        this->d_value += other.value();
    }
    else { // The right-hand value overwrites the left
        this->d_value = other.value();
        this->d_is_delta = false;
    }

    return *this;
}

bool GaugeMetricValue::operator==(const GaugeMetricValue &other) const
{
    return this->value() == other.value() &&
           this->isDelta() == other.isDelta();
}

bool GaugeMetricValue::operator!=(const GaugeMetricValue &other) const
{
    return !(*this == other);
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
