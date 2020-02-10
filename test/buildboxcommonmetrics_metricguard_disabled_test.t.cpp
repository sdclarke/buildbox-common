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

#include <buildboxcommonmetrics_metricguard.h>
#include <gtest/gtest.h>

class MockValueType {
  private:
  public:
    static const bool isAggregatable = false;
    MockValueType() {}
    int value() { return -1; }
};

class MockTimer {
    typedef MockValueType ValueType;

  private:
    MockValueType d_valueType;
    const std::string d_name;
    bool d_started = false;
    bool d_stopped = false;

  public:
    MockTimer(const std::string &name) : d_name(name), d_valueType() {}
    void start()
    {
        EXPECT_EQ(false, d_started);
        d_started = true;
    }
    void stop()
    {
        EXPECT_EQ(false, d_stopped);
        d_stopped = true;
    }
    const std::string &name() { return d_name; }
    MockValueType value() { return d_valueType; }
};

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, MetricGuardTestIgnoreLocallyDisabled)
{
    EXPECT_EQ(0, MetricCollectorFactory::getCollector<MockValueType>()
                     ->getIterableContainer()
                     ->size());
    { // scoped to check guard
        EXPECT_EQ(0, MetricCollectorFactory::getCollector<MockValueType>()
                         ->getIterableContainer()
                         ->size());

        EXPECT_TRUE(MetricCollectorFactory::getInstance()->metricsEnabled());
        MetricGuard<MockTimer> mg("test-metric", false);
    }
    // As of February 2020 the flag on the MetricGuard is ignored (see
    // DEPRECATED comment in buildboxcommonmetrics_metricguard.h)
    // But we left metrics globally enabled.
    EXPECT_EQ(1, MetricCollectorFactory::getCollector<MockValueType>()
                     ->getIterableContainer()
                     ->size());
    MetricCollectorFactory::getCollector<MockValueType>()
        ->getIterableContainer()
        ->clear();
}

TEST(MetricsTest, MetricGuardTestGloballyDisabled)
{
    MetricCollectorFactory::getInstance()
        ->getCollector<MockValueType>()
        ->getIterableContainer()
        ->clear();
    MetricCollectorFactory::getInstance()->disableMetrics();
    EXPECT_EQ(0, MetricCollectorFactory::getCollector<MockValueType>()
                     ->getIterableContainer()
                     ->size());
    { // scoped to check guard
        EXPECT_EQ(0, MetricCollectorFactory::getCollector<MockValueType>()
                         ->getIterableContainer()
                         ->size());

        MetricGuard<MockTimer> mg("test-metric", false);
        MetricGuard<MockTimer> mg2("test-metric", true);
    }
    // As of February 2020 the flag on the MetricGuard is ignored (see
    // DEPRECATED comment in buildboxcommonmetrics_metricguard.h)
    EXPECT_EQ(0, MetricCollectorFactory::getCollector<MockValueType>()
                     ->getIterableContainer()
                     ->size());
    MetricCollectorFactory::getInstance()->enableMetrics();
}
