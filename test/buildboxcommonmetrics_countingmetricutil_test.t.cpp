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
#include <buildboxcommonmetrics_metriccollectorfactory.h>
#include <buildboxcommonmetrics_metricguard.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, SimpleOnelineMetricPublish)
{
    // All these tests are in a single function as they will not behave well
    // when running in parallel.
    CountingMetricUtil::recordCounterMetric("namehere", 3);
    ASSERT_EQ(1, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->size());
    EXPECT_EQ(3, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->begin()
                     ->second.value());
    CountingMetricUtil::recordCounterMetric("namehere",
                                            CountingMetricValue(5));
    EXPECT_EQ(8, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->begin()
                     ->second.value());

    CountingMetricUtil::recordCounterMetric("foo", CountingMetricValue(5));
    EXPECT_EQ(2, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->size());

    CountingMetricUtil::recordCounterMetric(
        CountingMetric("foo", CountingMetricValue(5)));
    EXPECT_EQ(2, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->size());

    MetricCollectorFactory::getCollector<CountingMetricValue>()
        ->getIterableContainer()
        ->clear();

    CountingMetricUtil::recordCounterMetric(
        CountingMetric("other", CountingMetricValue(5)));
    CountingMetricUtil::recordCounterMetric(
        CountingMetric("other", CountingMetricValue(5)));
    CountingMetric other("other");
    other.setValue(42);
    CountingMetricUtil::recordCounterMetric(other);

    ASSERT_EQ(1, MetricCollectorFactory::getCollector<CountingMetricValue>()
                     ->getIterableContainer()
                     ->size());
    EXPECT_EQ(52, MetricCollectorFactory::getCollector<CountingMetricValue>()
                      ->getIterableContainer()
                      ->begin()
                      ->second.value());
}
