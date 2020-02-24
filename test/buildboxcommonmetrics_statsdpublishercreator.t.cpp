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
#include <buildboxcommonmetrics_statsdpublishercreator.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(MetricsConfigTest, GetStatsDPublisherFromConfig)
{

    EXPECT_THROW(
        buildboxcommonmetrics::MetricsConfigurator::createMetricsConfig(
            "/tmp/metrics", "localhost:3000", true),
        std::runtime_error);

    auto config =
        buildboxcommonmetrics::MetricsConfigurator::createMetricsConfig("", "",
                                                                        true);

    EXPECT_NO_THROW(
        buildboxcommonmetrics::StatsdPublisherCreator::createStatsdPublisher(
            config));

    config.setUdpServer("");
    config.setFile("/tmp/metrics");

    auto publisherType =
        buildboxcommonmetrics::StatsdPublisherCreator::createStatsdPublisher(
            config);

    ASSERT_EQ(publisherType->publishPath(), config.file());
    ASSERT_EQ(
        publisherType->publishMethod(),
        buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::File);

    config.setUdpServer("localhost:3000");
    config.setFile("");

    auto publisherType2 =
        buildboxcommonmetrics::StatsdPublisherCreator::createStatsdPublisher(
            config);

    ASSERT_EQ(publisherType2->publishPort(), 3000);
    ASSERT_EQ(
        publisherType2->publishMethod(),
        buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::UDP);
}

TEST(MetricsConfigTest, ParseHostPortFromStringTest)
{
    const uint16_t defaultStatsDPort = 8125;
    std::string host;
    uint16_t port;

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "localhost:1234", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(port, 1234);

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "localhost:", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(defaultStatsDPort, port);

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "localhost", &host, &port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(defaultStatsDPort, port);

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "somehost:6789", &host, &port);
    EXPECT_EQ("somehost", host);
    EXPECT_EQ(6789, port);

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "127.0.0.1:6789", &host, &port);
    EXPECT_EQ("127.0.0.1", host);
    EXPECT_EQ(6789, port);

    buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
        "example.org:6789", &host, &port);
    EXPECT_EQ("example.org", host);
    EXPECT_EQ(6789, port);
}

TEST(MetricsConfigTest, InvalidPort)
{
    const uint32_t badPort = std::numeric_limits<uint16_t>::max() + 1;
    std::string host;
    uint16_t port = 0;
    EXPECT_THROW(
        buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
            "example.org:" + std::to_string(badPort), &host, &port),
        std::range_error);

    EXPECT_NO_THROW(
        buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
            "example.org:" + std::to_string(badPort - 1), &host, &port));
    EXPECT_EQ(badPort - 1, port);
}

TEST(MetricsConfigTest, NullArgs)
{
    EXPECT_THROW(
        buildboxcommonmetrics::StatsdPublisherCreator::parseHostPortString(
            "example.org:1234", nullptr, nullptr),
        std::invalid_argument);
}
