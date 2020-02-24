// Copyright 2020 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this setFile except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <buildboxcommonmetrics_metricsconfigurator.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(MetricsConfigTest, IsMetricsOption)
{
    EXPECT_TRUE(buildboxcommonmetrics::MetricsConfigurator::isMetricsOption(
        "metrics-flag"));
    EXPECT_TRUE(buildboxcommonmetrics::MetricsConfigurator::isMetricsOption(
        "metrics-flag-metric"));
    EXPECT_FALSE(buildboxcommonmetrics::MetricsConfigurator::isMetricsOption(
        "a-metrics-flag-metric"));
    EXPECT_FALSE(buildboxcommonmetrics::MetricsConfigurator::isMetricsOption(
        "Metrics-InCaps"));
    EXPECT_FALSE(
        buildboxcommonmetrics::MetricsConfigurator::isMetricsOption(""));
}

TEST(MetricConfigTest, ParseMetric)
{
    buildboxcommonmetrics::MetricsConfigType config;
    auto metric_mode = "metrics-mode";
    auto metric_interval = "metrics-publish-interval";

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
        metric_mode, "stderr://", &config));
    EXPECT_EQ(config.enable(), true);

    auto metric_file = "file:///tmp/file";
    auto metric_udp = "udp://localhost:5000";

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
        metric_mode, metric_file, &config));

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
        metric_mode, metric_udp, &config));

    EXPECT_EQ(config.enable(), true);
    EXPECT_EQ(config.file(), "/tmp/file");
    EXPECT_EQ(config.udp_server(), "localhost:5000");

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
                     "metric-not-option", "/tmp/file", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
                     metric_mode, "", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
                     metric_mode, "udp://", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
                     metric_mode, "mode:", &config),
                 std::runtime_error);

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigurator::metricsParser(
        metric_interval, "60", &config));
    EXPECT_EQ(config.interval(), 60);
}
