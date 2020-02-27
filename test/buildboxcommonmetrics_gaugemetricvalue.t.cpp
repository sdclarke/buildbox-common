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
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, GaugeMetricValueAbsoluteByDefault)
{
    GaugeMetricValue v(1234);
    EXPECT_EQ(v.value(), 1234);
    EXPECT_FALSE(v.isDelta());
}

TEST(MetricsTest, GaugeMetricDeltaValue)
{
    GaugeMetricValue dv(5678, true);
    EXPECT_EQ(dv.value(), 5678);
    EXPECT_TRUE(dv.isDelta());
}

TEST(MetricsTest, GaugeMetricAbsoluteValueStatsD)
{
    GaugeMetricValue v(1024, false);
    EXPECT_EQ(v.toStatsD("gauge-metric-name"), "gauge-metric-name:1024|g");
}

TEST(MetricsTest, GaugeMetricNegativeAbsoluteValueStatsD)
{
    GaugeMetricValue v(-10, false);
    EXPECT_EQ(v.toStatsD("gauge-metric-name"),
              "gauge-metric-name:0|g\ngauge-metric-name:-10|g");
}

TEST(MetricsTest, GaugeMetricPositiveDeltaStatsD)
{
    GaugeMetricValue v(3, true);
    EXPECT_EQ(v.toStatsD("gauge-metric-name"), "gauge-metric-name:+3|g");
}

TEST(MetricsTest, GaugeMetricNegativeDeltaStatsD)
{
    GaugeMetricValue v(-5, true);
    EXPECT_EQ(v.toStatsD("gauge-metric-name"), "gauge-metric-name:-5|g");
}

TEST(MetricsTest, GaugeMetricValueValuePlusValue)
{
    GaugeMetricValue v(10);
    v += GaugeMetricValue(20);

    ASSERT_FALSE(v.isDelta());
    ASSERT_EQ(v.value(), 20); // overwritten
}

TEST(MetricsTest, GaugeMetricValueDeltaPlusValue)
{
    GaugeMetricValue v(1, true);
    v += GaugeMetricValue(4);

    ASSERT_FALSE(v.isDelta());
    ASSERT_EQ(v.value(), 4); // overwritten
}

TEST(MetricsTest, GaugeMetricValueValuePlusDelta)
{
    GaugeMetricValue v(10);
    v += GaugeMetricValue(1, true);

    ASSERT_FALSE(v.isDelta());
    ASSERT_EQ(v.value(), 11); // added
}

TEST(MetricsTest, GaugeMetricValueDeltaPlusDelta)
{
    GaugeMetricValue v(1, true);
    v += GaugeMetricValue(-3, true);

    ASSERT_TRUE(v.isDelta());
    ASSERT_EQ(v.value(), -2); // added
}

TEST(MetricsTest, GaugeMetricValueComparison)
{
    ASSERT_EQ(GaugeMetricValue(123, true), GaugeMetricValue(123, true));
    ASSERT_EQ(GaugeMetricValue(45, false), GaugeMetricValue(45));

    ASSERT_NE(GaugeMetricValue(1, true), GaugeMetricValue(1, false));
    ASSERT_NE(GaugeMetricValue(3, true), GaugeMetricValue(4, true));
    ASSERT_NE(GaugeMetricValue(5, true), GaugeMetricValue(6, false));
}
