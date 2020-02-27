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

#include <buildboxcommonmetrics_gaugemetric.h>
#include <buildboxcommonmetrics_metricguard.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, GaugeMetricConstructorFromGaugeValue)
{
    GaugeMetric g("my-gauge", GaugeMetricValue(45));
    EXPECT_EQ(g.name(), "my-gauge");
    EXPECT_EQ(g.value().value(), 45);
}

TEST(MetricsTest, GaugeMetricConstructorFromNegativeGaugeDelta)
{
    GaugeMetric g("my-gauge", GaugeMetricValue(-2, true));
    EXPECT_EQ(g.name(), "my-gauge");
    EXPECT_EQ(g.value().value(), -2);
}

TEST(MetricsTest, GaugeMetricConstructorFromPositiveGaugeDelta)
{
    GaugeMetric g("my-gauge", GaugeMetricValue(2, true));
    EXPECT_EQ(g.name(), "my-gauge");
    EXPECT_EQ(g.value().value(), 2);
}

TEST(MetricsTest, GaugeMetricCollected)
{
    MetricCollector<GaugeMetricValue> collector;
    collector.store("gauge1", GaugeMetricValue(22));

    const auto collected_metrics = collector.getSnapshot();
    EXPECT_EQ(collected_metrics.size(), 1);

    EXPECT_EQ(collected_metrics.cbegin()->first, "gauge1");
    EXPECT_EQ(collected_metrics.cbegin()->second.value(), 22);
}

TEST(MetricsTest, GaugeMetricPositiveDeltaCollected)
{
    MetricCollector<GaugeMetricValue> collector;
    collector.store("gauge1", GaugeMetricValue(2, true));

    const auto collected_metrics = collector.getSnapshot();
    EXPECT_EQ(collected_metrics.size(), 1);

    EXPECT_EQ(collected_metrics.cbegin()->first, "gauge1");
    EXPECT_EQ(collected_metrics.cbegin()->second.value(), 2);
}

TEST(MetricsTest, GaugeMetricNegativeDeltaCollected)
{
    MetricCollector<GaugeMetricValue> collector;
    collector.store("gauge1", GaugeMetricValue(-3, true));

    const auto collected_metrics = collector.getSnapshot();
    EXPECT_EQ(collected_metrics.size(), 1);

    EXPECT_EQ(collected_metrics.cbegin()->first, "gauge1");
    EXPECT_EQ(collected_metrics.cbegin()->second.value(), -3);
}
