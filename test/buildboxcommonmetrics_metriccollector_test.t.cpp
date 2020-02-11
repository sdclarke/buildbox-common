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

TEST(MetricsTest, DurationMetricValueCollectorUpdateTest)
{
    MetricCollector<DurationMetricValue> durationMetricCollector;

    DurationMetricValue myValue1(std::chrono::microseconds(2));
    durationMetricCollector.store("metric", myValue1);

    // We should only have 1 metric, with the value we instantiated it with
    // above
    auto metrics = durationMetricCollector.getSnapshot();
    ASSERT_EQ(1, metrics.size());
    ASSERT_EQ(std::chrono::microseconds(2),
              metrics.find("metric")->second.value());

    // Since DurationMetricValue is non-aggregatable,
    // storing the metric with the same name should *replace* it with the new
    // value
    DurationMetricValue myValue2(std::chrono::microseconds(5));
    durationMetricCollector.store("metric", myValue2);

    metrics = durationMetricCollector.getSnapshot();
    ASSERT_EQ(1, metrics.size());
    ASSERT_EQ(std::chrono::microseconds(5),
              metrics.find("metric")->second.value());
}

TEST(MetricsTest, TotalDurationMetricValueCollectorMultiTest)
{
    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;

    TotalDurationMetricValue myValue1;
    totalDurationMetricCollector.store("metric-1", myValue1);

    ASSERT_EQ(1, totalDurationMetricCollector.getSnapshot().size());

    TotalDurationMetricValue myValue2;
    totalDurationMetricCollector.store("metric-2", myValue1);

    ASSERT_EQ(1, totalDurationMetricCollector.getSnapshot().size());
}

TEST(MetricsTest, TotalDurationMetricValueCollectorAggregateTest)
{
    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;

    // add 2 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue1(std::chrono::microseconds(2));
    totalDurationMetricCollector.store("metric", myValue1);

    // add 5 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue2(std::chrono::microseconds(5));
    totalDurationMetricCollector.store("metric", myValue2);

    auto metrics = totalDurationMetricCollector.getSnapshot();
    ASSERT_EQ(1, metrics.size());
    ASSERT_EQ(std::chrono::microseconds(7),
              metrics.find("metric")->second.value());
}

TEST(MetricsTest, TotalDurationMetricValueCollectorMultiAggregateTest)
{
    MetricCollector<TotalDurationMetricValue> totalDurationMetricCollector;

    // add 2 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue1(std::chrono::microseconds(2));
    totalDurationMetricCollector.store("metric", myValue1);

    // add 4 seconds to a metric named 'metric-other'
    TotalDurationMetricValue myValueOther1(std::chrono::microseconds(4));
    totalDurationMetricCollector.store("metric-other", myValueOther1);

    // add 5 seconds to a metric named 'metric'
    TotalDurationMetricValue myValue2(std::chrono::microseconds(5));
    totalDurationMetricCollector.store("metric", myValue2);

    // add 9 seconds to a metric named 'metric-other'
    TotalDurationMetricValue myValueOther2(std::chrono::microseconds(9));
    totalDurationMetricCollector.store("metric-other", myValueOther2);

    // confirm that we have 2 entries --> 'metric' and 'metric-other'
    auto metrics = totalDurationMetricCollector.getSnapshot();
    ASSERT_EQ(2, metrics.size());
    ASSERT_EQ(std::chrono::microseconds(7),
              metrics.find("metric")->second.value());
    ASSERT_EQ(std::chrono::microseconds(13),
              metrics.find("metric-other")->second.value());
}
