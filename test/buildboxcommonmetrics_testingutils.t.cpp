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

#include <buildboxcommonmetrics_countingmetricutil.h>
#include <buildboxcommonmetrics_durationmetrictimer.h>
#include <buildboxcommonmetrics_gaugemetricutil.h>
#include <buildboxcommonmetrics_metriccollector.h>
#include <buildboxcommonmetrics_metriccollectorfactoryutil.h>
#include <buildboxcommonmetrics_testingutils.h>
#include <buildboxcommonmetrics_totaldurationmetrictimer.h>

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

TEST_F(MetricTestingUtilsTest, ValidateMetricValuesMissing)
{
    d_collector->store("metric400", MockMetricValue(400));

    // this returns true now because metric500 was not collected
    ASSERT_TRUE(validateMetricCollection<MockMetricValue>(
        {{"metric400", MockMetricValue(400)}}, {"metric500"}));
}

TEST_F(MetricTestingUtilsTest, ValidateMetricValuesMissingFail)
{
    d_collector->store("metric400", MockMetricValue(400));
    d_collector->store("metric500", MockMetricValue(500));

    // this returns false because metric500 does appear.
    ASSERT_FALSE(validateMetricCollection<MockMetricValue>(
        {{"metric400", MockMetricValue(400)}}, {"metric500"}));
}

TEST_F(MetricTestingUtilsTest, ClearMetricValues)
{

    const auto metric_name = "metric123";
    const auto metric_value = MockMetricValue(123);

    d_collector->store(metric_name, metric_value);
    clearMetricCollection<MockMetricValue>();

    ASSERT_FALSE(
        validateMetricCollection<MockMetricValue>(metric_name, metric_value));
}

TEST_F(MetricTestingUtilsTest, AddMetricsAfterClearing)
{

    const auto metric_name = "metric123";
    const auto metric_value = MockMetricValue(123);

    clearMetricCollection<MockMetricValue>();
    d_collector->store(metric_name, metric_value);

    ASSERT_TRUE(
        validateMetricCollection<MockMetricValue>(metric_name, metric_value));
}

TEST_F(MetricTestingUtilsTest, ClearAllMetricsTest)
{

    const auto gauge_metric_name = "metricGauge";
    const auto count_metric_name = "metricCount";
    const auto duration_metric_name = "metricDuration";
    const auto total_duration_metric_name = "metricTotalDuration";
    const int metric_value = 123;

    // insert metrics
    CountingMetricUtil::recordCounterMetric(count_metric_name, metric_value);
    MetricCollectorFactoryUtil::store(duration_metric_name,
                                      DurationMetricValue());
    MetricCollectorFactoryUtil::store(total_duration_metric_name,
                                      TotalDurationMetricValue());
    GaugeMetricUtil::setGauge(gauge_metric_name, metric_value);

    // clear all metrics
    clearAllMetricCollection();

    // verify all metrics have been cleared
    EXPECT_FALSE(validateMetricCollection<CountingMetric>(count_metric_name));
    EXPECT_FALSE(
        validateMetricCollection<DurationMetricTimer>(duration_metric_name));
    EXPECT_FALSE(validateMetricCollection<TotalDurationMetricTimer>(
        total_duration_metric_name));
    EXPECT_FALSE(validateMetricCollection<GaugeMetric>(gauge_metric_name));
}

TEST_F(MetricTestingUtilsTest, ClearAllMetricsTestBeforeInserts)
{

    const auto gauge_metric_name = "metricGauge";
    const auto count_metric_name = "metricCount";
    const auto duration_metric_name = "metricDuration";
    const auto total_duration_metric_name = "metricTotalDuration";
    const int metric_value = 123;

    // clear all metrics
    clearAllMetricCollection();

    // insert metrics
    CountingMetricUtil::recordCounterMetric(count_metric_name, metric_value);
    MetricCollectorFactoryUtil::store(duration_metric_name,
                                      DurationMetricValue());
    MetricCollectorFactoryUtil::store(total_duration_metric_name,
                                      TotalDurationMetricValue());
    GaugeMetricUtil::setGauge(gauge_metric_name, metric_value);

    // verify recently inserted metrics have not been cleared
    EXPECT_TRUE(validateMetricCollection<CountingMetric>(count_metric_name));
    EXPECT_TRUE(
        validateMetricCollection<DurationMetricTimer>(duration_metric_name));
    EXPECT_TRUE(validateMetricCollection<TotalDurationMetricTimer>(
        total_duration_metric_name));
    EXPECT_TRUE(validateMetricCollection<GaugeMetric>(gauge_metric_name));
}

TEST_F(MetricTestingUtilsTest, ValidateSingleMetricMultipleValuesPositive)
{
    const std::vector<std::pair<std::string, MockMetricValue>> metrics = {
        std::make_pair("metric1", MockMetricValue(1)),
        std::make_pair("metric1", MockMetricValue(2)),
    };

    for (const auto &metric_pair : metrics) {
        d_collector->store(metric_pair.first, metric_pair.second);
    }

    ASSERT_TRUE(validateMetricCollection<MockMetricValue>(metrics));
}

TEST_F(MetricTestingUtilsTest, ValidateSingleMetricMultipleValuesNegative)
{
    const std::vector<std::pair<std::string, MockMetricValue>> metrics = {
        std::make_pair("metric2", MockMetricValue(3)),
        std::make_pair("metric2", MockMetricValue(4)),
    };

    for (const auto &metric_pair : metrics) {
        d_collector->store(metric_pair.first, metric_pair.second);
    }

    const std::vector<std::pair<std::string, MockMetricValue>> wrong_metrics =
        {
            std::make_pair("metric2", MockMetricValue(3)),
            std::make_pair("metric2", MockMetricValue(5)),
        };

    ASSERT_FALSE(validateMetricCollection<MockMetricValue>(wrong_metrics));
}
