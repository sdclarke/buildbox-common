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

#include <buildboxcommon_metricsconfig.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(MetricsConfigTest, ParseHostPortFromStringTest)
{
    std::string host;
    int port;

    buildboxcommonmetrics::MetricsConfig config;

    config.parseHostPortString("localhost:1234", host, port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(port, 1234);

    config.parseHostPortString("localhost:", host, port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(0, port);

    config.parseHostPortString("localhost", host, port);
    EXPECT_EQ("localhost", host);
    EXPECT_EQ(0, port);

    config.parseHostPortString("somehost:6789", host, port);
    EXPECT_EQ("somehost", host);
    EXPECT_EQ(6789, port);

    config.parseHostPortString("127.0.0.1:6789", host, port);
    EXPECT_EQ("127.0.0.1", host);
    EXPECT_EQ(6789, port);

    config.parseHostPortString("example.org:6789", host, port);
    EXPECT_EQ("example.org", host);
    EXPECT_EQ(6789, port);
}

TEST(MetricsConfigTest, GetStatsDPublisherFromConfig)
{

    buildboxcommonmetrics::MetricsConfig config("/tmp/metrics",
                                                "localhost:3000", true);

    EXPECT_THROW(config.getStatsdPublisherFromConfig(), std::runtime_error);

    config.setUdpServer("");
    config.setFile("");

    EXPECT_NO_THROW(config.getStatsdPublisherFromConfig());

    config.setUdpServer("");
    config.setFile("/tmp/metrics");

    auto publisherType = config.getStatsdPublisherFromConfig();

    ASSERT_EQ(publisherType.publishPath(), config.file());
    ASSERT_EQ(
        publisherType.publishMethod(),
        buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::File);

    config.setUdpServer("localhost:3000");
    config.setFile("");

    auto publisherType2 = config.getStatsdPublisherFromConfig();

    ASSERT_EQ(publisherType2.publishPort(), 3000);
    ASSERT_EQ(
        publisherType2.publishMethod(),
        buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::UDP);
}
