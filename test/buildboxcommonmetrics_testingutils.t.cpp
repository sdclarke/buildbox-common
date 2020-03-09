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

#include <buildboxcommonmetrics_metriccollector.h>
#include <buildboxcommonmetrics_testingutils.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

class MockMetricValue {
  public:
    static const bool isAggregatable = false;

    MockMetricValue(const int value) : d_value(value) {}

    int value() const { return d_value; }

    bool operator==(const MockMetricValue &other) const
    {
        return this->value() == other.value();
    }

    bool operator!=(const MockMetricValue &other) const
    {
        return this->value() != other.value();
    }

  private:
    int d_value;
};

class MockMetric {
  public:
    MockMetric(const MockMetricValue &value) : d_value(value) {}

    MockMetricValue value() const { return d_value; }

    bool operator==(const MockMetric &other) const
    {
        return this->value() == other.value();
    }

    bool operator!=(const MockMetric &other) const
    {
        return this->value() != other.value();
    }

  private:
    const MockMetricValue d_value;
};

class MetricTestingUtilsTest : public ::testing::Test {
  protected:
    MetricTestingUtilsTest()
        : d_collector(MetricCollectorFactory::getCollector<MockMetricValue>())
    {
    }

    MetricCollector<MockMetricValue> *d_collector;
};

TEST_F(MetricTestingUtilsTest, ValidateSingleMetric)
{
    d_collector->store("metric1", MockMetricValue(1));

    ASSERT_TRUE(validateMetricCollection<MockMetric>("metric1"));

    ASSERT_FALSE(validateMetricCollection<MockMetric>("metric10"));
}

TEST_F(MetricTestingUtilsTest, ValidateMultipleMetricsPositive)
{
    d_collector->store("metric1", MockMetricValue(1));
    d_collector->store("metric2", MockMetricValue(2));

    const std::vector<std::string> metrics = {"metric1", "metric2"};
    ASSERT_TRUE(validateMetricCollection<MockMetric>(metrics));
}

TEST_F(MetricTestingUtilsTest, ValidateMultipleMetricsNegative)
{

    d_collector->store("metric3", MockMetricValue(3));
    d_collector->store("metric4", MockMetricValue(4));

    const std::vector<std::string> metrics = {"metric3", "metric4", "metric5"};
    ASSERT_FALSE(validateMetricCollection<MockMetric>(metrics));
}

TEST_F(MetricTestingUtilsTest, ValidateMetricValue)
{

    const auto metric_name = "metric123";
    const auto metric_value = MockMetricValue(123);

    d_collector->store(metric_name, metric_value);

    ASSERT_TRUE(
        validateMetricCollection<MockMetricValue>(metric_name, metric_value));
    // (This version takes the metric's `ValueType` directly)
}

TEST_F(MetricTestingUtilsTest, ValidateMetricValuesPositive)
{

    const std::vector<std::pair<std::string, MockMetricValue>> entries = {
        {"metric100", MockMetricValue(100)},
        {"metric200", MockMetricValue(200)},
        {"metric300", MockMetricValue(300)}};

    for (const auto &entry : entries) {
        d_collector->store(entry.first, entry.second);
    }

    ASSERT_TRUE(validateMetricCollection<MockMetricValue>(entries));
    // (This version takes the metric's `ValueType` directly)
}

TEST_F(MetricTestingUtilsTest, ValidateMetricValuesPositiveNegative)
{

    d_collector->store("metric400", MockMetricValue(400));

    ASSERT_FALSE(validateMetricCollection<MockMetricValue>(
        {{"metric400", MockMetricValue(400)},
         {"metric500", MockMetricValue(500)}}));
    // (This version takes the metric's `ValueType` directly)
}
