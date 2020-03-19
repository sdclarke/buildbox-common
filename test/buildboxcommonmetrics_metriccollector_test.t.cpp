// Copyright 2019 Bloomberg Finance L.P
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

#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_metriccollector.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, DurationMetricValueCollectorMultiTest)
{
    MetricCollector<DurationMetricValue> durationMetricCollector;

    DurationMetricValue myValue1;
    durationMetricCollector.store("metric-1", myValue1);

    ASSERT_EQ(1, durationMetricCollector.getSnapshot().size());

    DurationMetricValue myValue2;
    durationMetricCollector.store("metric-2", myValue1);

    ASSERT_EQ(1, durationMetricCollector.getSnapshot().size());
}

TEST(MetricsTest, DurationMetricValueCollectorMultipleNonAggregatableEntries)
{
    const bool isAggregatableType = DurationMetricValue::isAggregatable;
    const std::string metric_name = "metric";
    ASSERT_FALSE(isAggregatableType);

    MetricCollector<DurationMetricValue> durationMetricCollector;

    DurationMetricValue myValue1(std::chrono::microseconds(2));
    durationMetricCollector.store(metric_name, myValue1);

    // Since `DurationMetricValue` is non-aggregatable, storing the metric with
    // the same name should produce two entries.
    DurationMetricValue myValue2(std::chrono::microseconds(5));
    durationMetricCollector.store(metric_name, myValue2);

    const auto snapshot = durationMetricCollector.getSnapshot();
    ASSERT_EQ(2, snapshot.size());

    const auto first_metric = std::find_if(
        snapshot.begin(), snapshot.end(),
        [&metric_name](const std::pair<std::string, DurationMetricValue>
                           &metric_key_value_pair) {
            return metric_key_value_pair.first == metric_name;
        });
    ASSERT_NE(first_metric, snapshot.cend());
    ASSERT_EQ(std::chrono::microseconds(2), first_metric->second.value());

    const auto second_metric = std::find_if(
        first_metric + 1, snapshot.end(),
        [&metric_name](const std::pair<std::string, DurationMetricValue>
                           &metric_key_value_pair) {
            return metric_key_value_pair.first == metric_name;
        });
    ASSERT_NE(second_metric, snapshot.cend());
    ASSERT_EQ(second_metric->second.value(), std::chrono::microseconds(5));
}

TEST(MetricsTest, TotalDurationMetricValueCollectorMultiTest)
{
    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;

    TotalDurationMetricValue myValue1;
    totalDurationMetricCollector.store("metric-1", myValue1);

    TotalDurationMetricValue myValue2;
    totalDurationMetricCollector.store("metric-2", myValue1);

    ASSERT_EQ(2, totalDurationMetricCollector.getSnapshot().size());
}

TEST(MetricsTest, TotalDurationMetricValueCollectorAggregateTest)
{
    const bool isAggregatableType = TotalDurationMetricValue::isAggregatable;
    ASSERT_TRUE(isAggregatableType);

    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;
    const std::string metric_name = "metric";

    // add 2 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue1(std::chrono::microseconds(2));
    totalDurationMetricCollector.store(metric_name, myValue1);

    // add 5 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue2(std::chrono::microseconds(5));
    totalDurationMetricCollector.store(metric_name, myValue2);

    auto metrics = totalDurationMetricCollector.getSnapshot();
    ASSERT_EQ(metrics.size(), 1);

    const auto collected_metric =
        metrics.begin(); // should be the first and only metric

    ASSERT_EQ(collected_metric->first, metric_name);
    ASSERT_EQ(collected_metric->second.value(), std::chrono::microseconds(7));
}

TEST(MetricsTest, TotalDurationMetricValueCollectorMultiAggregateTest)
{
    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;

    const std::string metric_name = "metric";
    const std::string metric_name_other = "metric-other";

    // add 2 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue1(std::chrono::microseconds(2));
    totalDurationMetricCollector.store(metric_name, myValue1);

    // add 4 seconds to a metric named 'metric-other'
    TotalDurationMetricValue myValueOther1(std::chrono::microseconds(4));
    totalDurationMetricCollector.store(metric_name_other, myValueOther1);

    // add 5 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue2(std::chrono::microseconds(5));
    totalDurationMetricCollector.store(metric_name, myValue2);

    // add 9 seconds to a metric named 'metric-other'
    TotalDurationMetricValue myValueOther2(std::chrono::microseconds(9));
    totalDurationMetricCollector.store(metric_name_other, myValueOther2);

    // confirm that we have 2 entries --> 'metric' and 'metric-other'
    const auto metrics = totalDurationMetricCollector.getSnapshot();
    ASSERT_EQ(metrics.size(), 2);

    const auto first_metric = std::find_if(
        metrics.begin(), metrics.end(),
        [&metric_name](const std::pair<std::string, TotalDurationMetricValue>
                           &metric_key_value_pair) {
            return metric_key_value_pair.first == metric_name;
        });
    ASSERT_NE(first_metric, metrics.cend());
    ASSERT_EQ(first_metric->second.value(), std::chrono::microseconds(7));

    const auto second_metric = std::find_if(
        metrics.begin(), metrics.end(),
        [&metric_name_other](
            const std::pair<std::string, TotalDurationMetricValue>
                &metric_key_value_pair) {
            return metric_key_value_pair.first == metric_name_other;
        });
    ASSERT_NE(second_metric, metrics.cend());
    ASSERT_EQ(second_metric->second.value(), std::chrono::microseconds(13));
}
