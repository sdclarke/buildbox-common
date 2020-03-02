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

TEST(StatsDPublisherCreatorTest, CreateStatsDPublisherFromConfig)
{
    auto config =
        buildboxcommonmetrics::MetricsConfigurator::createMetricsConfig("", "",
                                                                        true);

    auto publisherType =
        buildboxcommonmetrics::StatsdPublisherCreator::createStatsdPublisher(
            config);
}
