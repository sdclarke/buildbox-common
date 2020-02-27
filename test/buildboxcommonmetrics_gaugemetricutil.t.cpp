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

#include <buildboxcommonmetrics_gaugemetricutil.h>
#include <buildboxcommonmetrics_metriccollectorfactory.h>

#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, SimpleOnelineMetricPublish)
{
    // All these tests are in a single function as they will not behave well
    // when running in parallel.

    auto *collector = MetricCollectorFactory::getCollector<GaugeMetricValue>();

    {
        GaugeMetricUtil::setGauge("gauge0", 3);
        GaugeMetricUtil::adjustGauge("gauge1", -5);
        GaugeMetricUtil::adjustGauge("gauge0", -1);

        const auto snapshot = collector->getSnapshot();
        ASSERT_EQ(2, snapshot.size());
        ASSERT_EQ(snapshot.count("gauge0"), 1);
        ASSERT_EQ(snapshot.count("gauge1"), 1);

        ASSERT_EQ(snapshot.at("gauge0").value(), 3 - 1);
        ASSERT_EQ(snapshot.at("gauge1").value(), -5);
    }

    {
        const GaugeMetric g2("gauge2", GaugeMetricValue(1024));
        GaugeMetricUtil::recordGauge(g2);

        GaugeMetricUtil::adjustGauge("gauge3", 2);
        GaugeMetricUtil::setGauge("gauge3", 25);

        const auto snapshot = collector->getSnapshot();
        ASSERT_EQ(2, snapshot.size());
        ASSERT_EQ(snapshot.count("gauge2"), 1);
        ASSERT_EQ(snapshot.count("gauge3"), 1);

        ASSERT_EQ(snapshot.at("gauge2").value(), 1024);
        ASSERT_EQ(snapshot.at("gauge3").value(), 25);
    }

    {
        GaugeMetricUtil::setGauge("gauge3", -10);

        const auto snapshot = collector->getSnapshot();
        ASSERT_EQ(1, snapshot.size());
        ASSERT_EQ(snapshot.count("gauge3"), 1);

        ASSERT_EQ(snapshot.at("gauge3").value(), -10);
    }
}
