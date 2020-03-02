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

#include <buildboxcommonmetrics_metricsconfigutil.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(MetricsConfigUtilTest, IsMetricsOption)
{
    EXPECT_TRUE(buildboxcommonmetrics::MetricsConfigUtil::isMetricsOption(
        "metrics-flag"));
    EXPECT_TRUE(buildboxcommonmetrics::MetricsConfigUtil::isMetricsOption(
        "metrics-flag-metric"));
    EXPECT_FALSE(buildboxcommonmetrics::MetricsConfigUtil::isMetricsOption(
        "a-metrics-flag-metric"));
    EXPECT_FALSE(buildboxcommonmetrics::MetricsConfigUtil::isMetricsOption(
        "Metrics-InCaps"));
    EXPECT_FALSE(
        buildboxcommonmetrics::MetricsConfigUtil::isMetricsOption(""));
}

TEST(MetricConfigTest, ParseMetric)
{
    buildboxcommonmetrics::MetricsConfigType config;
    auto metric_mode = "metrics-mode";
    auto metric_interval = "metrics-publish-interval";

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
        metric_mode, "stderr://", &config));
    EXPECT_EQ(config.enable(), true);

    auto metric_file = "file:///tmp/file";
    auto metric_udp = "udp://localhost:5000";

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
        metric_mode, metric_file, &config));

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
        metric_mode, metric_udp, &config));

    EXPECT_EQ(config.enable(), true);
    EXPECT_EQ(config.file(), "/tmp/file");
    EXPECT_EQ(config.udp_server(), "localhost:5000");

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
                     "metric-not-option", "/tmp/file", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
                     metric_mode, "", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
                     metric_mode, "udp://", &config),
                 std::runtime_error);

    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
                     metric_mode, "mode:", &config),
                 std::runtime_error);

    EXPECT_NO_THROW(buildboxcommonmetrics::MetricsConfigUtil::metricsParser(
        metric_interval, "60", &config));
    EXPECT_EQ(config.interval(), 60);
}

TEST(MetricsConfigUtilTest, ParseHostPortFromStringTest)
{
    const uint16_t defaultStatsDPort = 8125;
    std::string host;
    uint16_t port;

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "localhost:1234", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(port, 1234);

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "localhost:", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(defaultStatsDPort, port);

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "localhost", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(defaultStatsDPort, port);

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "somehost:6789", &host, &port);
    EXPECT_EQ("somehost", host);
    EXPECT_EQ(6789, port);

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "127.0.0.1:6789", &host, &port);
    EXPECT_EQ("127.0.0.1", host);
    EXPECT_EQ(6789, port);

    buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
        "example.org:6789", &host, &port);
    EXPECT_EQ("example.org", host);
    EXPECT_EQ(6789, port);
}

TEST(MetricsConfigUtilTest, InvalidPort)
{
    const uint32_t badPort = std::numeric_limits<uint16_t>::max() + 1;
    std::string host;
    uint16_t port = 0;
    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
                     "example.org:" + std::to_string(badPort), &host, &port),
                 std::range_error);

    EXPECT_NO_THROW(
        buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
            "example.org:" + std::to_string(badPort - 1), &host, &port));
    EXPECT_EQ(badPort - 1, port);
}

TEST(MetricsConfigUtilTest, NullArgs)
{
    EXPECT_THROW(buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
                     "example.org:1234", nullptr, nullptr),
                 std::invalid_argument);
}
