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

#include <buildboxcommonmetrics_metricsconfigurator.h>
#include <buildboxcommonmetrics_statsdpublisher.h>

#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

using StatsDAllMetricsPublisher =
    MetricsConfigurator::publisherTypeOfAllValueTypes<StatsDPublisher>;

TEST(MetricsConfigTest, CreateMetricsConfigs)
{
    EXPECT_THROW(MetricsConfigurator::createMetricsConfig(
                     "/tmp/metrics", "localhost:3000", true),
                 std::runtime_error);

    auto config = MetricsConfigurator::createMetricsConfig("", "", true);

    config.setUdpServer("");
    config.setFile("/tmp/metrics");
}
