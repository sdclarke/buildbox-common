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

#include <buildboxcommonmetrics_distributionmetric.h>

#include <buildboxcommonmetrics_metricguard.h>
#include <gtest/gtest.h>

#include <algorithm>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, DistributionMetricConstructorFromValue)
{
    const auto distributionName = "distribution";
    const auto distributionValue = DistributionMetricValue(123);

    const DistributionMetric g(distributionName, distributionValue);
    EXPECT_EQ(g.name(), distributionName);
    EXPECT_EQ(g.value(), distributionValue);
}

TEST(MetricsTest, DistributionMetricCollected)
{

    const auto distributionName = "distribution1";
    const auto distributionValue = DistributionMetricValue(256);

    MetricCollector<DistributionMetricValue> collector;
    collector.store(distributionName, distributionValue);

    const auto collected_metrics = collector.getSnapshot();
    EXPECT_EQ(collected_metrics.size(), 1);

    EXPECT_EQ(collected_metrics.cbegin()->first, distributionName);
    EXPECT_EQ(collected_metrics.cbegin()->second, distributionValue);
}

TEST(MetricsTest, DistributionMetricsCollected)
{
    const auto distributionName = "distribution1";

    MetricCollector<DistributionMetricValue> collector;
    collector.store(distributionName, DistributionMetricValue(256));
    collector.store(distributionName, DistributionMetricValue(512));

    const auto collected_metrics = collector.getSnapshot();
    EXPECT_EQ(collected_metrics.size(), 2);

    EXPECT_EQ(collected_metrics.at(0).first, distributionName);
    EXPECT_EQ(collected_metrics.at(1).first, distributionName);

    const std::vector<DistributionMetricValue> expectedValues = {
        DistributionMetricValue(256), DistributionMetricValue(512)};

    std::vector<DistributionMetricValue> values;
    for (const auto &entries : collected_metrics) {
        values.push_back(entries.second);
    }

    EXPECT_TRUE(std::is_permutation(values.cbegin(), values.cend(),
                                    expectedValues.cbegin()));
}
