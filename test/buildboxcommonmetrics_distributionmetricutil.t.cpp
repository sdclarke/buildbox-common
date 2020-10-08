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

#include <buildboxcommonmetrics_distributionmetricutil.h>

#include <buildboxcommonmetrics_metriccollectorfactory.h>

#include <gtest/gtest.h>

#include <unordered_set>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, PublishTwoEntriesOfSameMetric)
{
    // All these tests are in a single function as they will not behave well
    // when running in parallel.
    DistributionMetricUtil::recordDistributionMetric("dist1", 3);
    DistributionMetricUtil::recordDistributionMetric(
        DistributionMetric("dist1", DistributionMetricValue(5)));

    const auto metrics =
        MetricCollectorFactory::getCollector<DistributionMetricValue>()
            ->getSnapshot();

    const std::set<DistributionMetricValue::DistributionMetricNumericType>
        expectedValues = {3, 5};

    ASSERT_EQ(metrics.size(), 2);

    EXPECT_EQ(metrics.at(0).first, "dist1");
    ASSERT_EQ(expectedValues.count(metrics.at(0).second.value()), 1);

    EXPECT_EQ(metrics.at(1).first, "dist1");
    ASSERT_EQ(expectedValues.count(metrics.at(1).second.value()), 1);
}

TEST(MetricsTest, PublishTwoDifferentMetrics)
{
    // All these tests are in a single function as they will not behave well
    // when running in parallel.
    DistributionMetricUtil::recordDistributionMetric("dist1", 1);
    DistributionMetricUtil::recordDistributionMetric(
        DistributionMetric("dist2", DistributionMetricValue(2)));

    const auto metrics =
        MetricCollectorFactory::getCollector<DistributionMetricValue>()
            ->getSnapshot();

    ASSERT_EQ(metrics.size(), 2);

    for (const auto &entry : metrics) {
        if (entry.first == "dist1") {
            EXPECT_EQ(entry.second.value(), 1);
        }
        else if (entry.first == "dist2") {
            EXPECT_EQ(entry.second.value(), 2);
        }
        else {
            FAIL() << "Unexpected metric name: [" << entry.first << "]";
        }
    }
}
