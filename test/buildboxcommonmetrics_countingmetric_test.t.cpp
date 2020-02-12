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
#include <buildboxcommonmetrics_metricguard.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, CountingMetricConstructorSetGet)
{
    CountingMetric myValue("name");
    EXPECT_EQ(myValue.value().value(), 0);
    myValue.start();
    EXPECT_EQ(myValue.value().value(), 1);
    myValue.stop();
    EXPECT_EQ(myValue.value().value(), 1);
}

TEST(MetricsTest, CountingMetricGuarded)
{
    MetricCollector<CountingMetricValue> collector;
    {
        MetricGuard<CountingMetric> counted("some-test", &collector);
    }
    EXPECT_EQ(1, collector.getIterableContainer()->size());
    EXPECT_EQ(1, collector.getIterableContainer()->begin()->second.value());
    EXPECT_EQ("some-test", collector.getIterableContainer()->begin()->first);
}

TEST(MetricsTest, CountingMetricWithMetricGuard)
{
    MetricCollector<CountingMetricValue> collector;
    {
        CountingMetric m("my-counted");
        // This starts with a count of 1.
        ScopedMetric<CountingMetric> mg(&m, &collector);
        {
            // Increment again to get to 2
            m++;
        }
    }
    ASSERT_EQ(1, collector.getIterableContainer()->size());
    EXPECT_EQ(2, collector.getIterableContainer()->begin()->second.value());
    EXPECT_EQ("my-counted", collector.getIterableContainer()->begin()->first);
}

TEST(MetricsTest, CountingMetricWithMetricGuardCustomValue)
{
    MetricCollector<CountingMetricValue> collector;
    {
        CountingMetric m("my-counted");
        // This starts with a count of 1.
        ScopedMetric<CountingMetric> mg(&m, &collector);
        {
            m.setValue(-22);
        }
    }
    ASSERT_EQ(1, collector.getIterableContainer()->size());
    EXPECT_EQ(-22, collector.getIterableContainer()->begin()->second.value());
    EXPECT_EQ("my-counted", collector.getIterableContainer()->begin()->first);
}
