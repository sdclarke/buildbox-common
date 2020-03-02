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
#include <buildboxcommonmetrics_statsdpublisher.h>

#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

using StatsDAllMetricsPublisher =
    MetricsConfigurator::publisherTypeOfAllValueTypes<StatsDPublisher>;

TEST(MetricsConfigStatsDTest, CreateStatsDPublisherFromConfig)
{
    EXPECT_THROW(MetricsConfigurator::createMetricsConfig(
                     "/tmp/metrics", "localhost:3000", true),
                 std::runtime_error);

    auto config = MetricsConfigurator::createMetricsConfig("", "", true);

    EXPECT_NO_THROW(MetricsConfigurator::createMetricsPublisherWithConfig<
                        StatsDAllMetricsPublisher>(config););

    config.setUdpServer("");
    config.setFile("/tmp/metrics");

    auto publisherType = MetricsConfigurator::createMetricsPublisherWithConfig<
        StatsDAllMetricsPublisher>(config);

    ASSERT_EQ(publisherType->publishPath(), config.file());
    ASSERT_EQ(publisherType->publishMethod(),
              StatsDPublisherOptions::PublishMethod::File);

    config.setUdpServer("localhost:3000");
    config.setFile("");

    auto publisherType2 =
        MetricsConfigurator::createMetricsPublisherWithConfig<
            StatsDAllMetricsPublisher>(config);

    ASSERT_EQ(publisherType2->publishPort(), 3000);
    ASSERT_EQ(publisherType2->publishMethod(),
              StatsDPublisherOptions::PublishMethod::UDP);
}
