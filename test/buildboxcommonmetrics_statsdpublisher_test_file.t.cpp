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

#include <buildboxcommonmetrics_durationmetrictimer.h>
#include <buildboxcommonmetrics_metriccollectorfactoryutil.h>
#include <buildboxcommonmetrics_statsdpublisher.h>
#include <buildboxcommonmetrics_statsdpublisheroptions.h>

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporaryfile.h>

#include <gtest/gtest.h>
#include <sstream>

using namespace buildboxcommon::buildboxcommonmetrics;

class MockValueType {
  private:
  public:
    static const bool isAggregatable = false;
    MockValueType() {}
    int value() { return -1; }
    const std::string toStatsD(const std::string &name) const { return name; }
};

class AnotherMockValueType1 : public MockValueType {
};

class AnotherMockValueType2 : public MockValueType {
};

TEST(MetricsTest, StatsDPublisherTestWriteToFile)
{
    buildboxcommon::TemporaryFile outputFile;
    const auto metricsFile = outputFile.name();

    // Initialize a StatsDPublisher for MockValueType
    StatsDPublisher<MockValueType> myPublisher(
        StatsDPublisherOptions::PublishMethod::File, metricsFile);

    // Publish
    myPublisher.publish();
    // Expect the file to be empty since there were no actual metrics
    EXPECT_EQ("", buildboxcommon::FileUtils::get_file_contents(metricsFile));

    // Store "my-metric"
    MetricCollectorFactoryUtil::store("my-metric", MockValueType());
    // Publish
    myPublisher.publish();
    // Expect to have `my-metric` published
    EXPECT_EQ("my-metric\n",
              buildboxcommon::FileUtils::get_file_contents(metricsFile));

    // Store "another-metric"
    MetricCollectorFactoryUtil::store("another-metric", MockValueType());
    // Publish
    myPublisher.publish();
    // Expect to have those two metrics
    const auto fileContents =
        buildboxcommon::FileUtils::get_file_contents(metricsFile);
    EXPECT_NE(std::string::npos, fileContents.find("my-metric"));
    EXPECT_NE(std::string::npos, fileContents.find("another-metric"));
}

TEST(MetricsTest, StatsDPublisherTestWriteToFile2ValueTypes)
{
    buildboxcommon::TemporaryFile metricsFile;
    const auto metricsFilename = metricsFile.name();

    // Initialize a StatsDPublisher for MockValueType
    StatsDPublisher<AnotherMockValueType1, AnotherMockValueType2> myPublisher(
        StatsDPublisherOptions::PublishMethod::File, metricsFilename);

    // Publish
    myPublisher.publish();
    // Expect the file to be empty since there were no actual metrics
    {
        const auto fileContents =
            buildboxcommon::FileUtils::get_file_contents(metricsFilename);
        EXPECT_EQ("", fileContents);
    }

    // Store "my-metric"
    MetricCollectorFactoryUtil::store("my-metric", AnotherMockValueType1());
    // Publish
    myPublisher.publish();
    // Expect to have `my-metric` published
    {
        const auto fileContents =
            buildboxcommon::FileUtils::get_file_contents(metricsFilename);
        EXPECT_EQ("my-metric\n", fileContents);
    }

    // Store "additional-metric"
    MetricCollectorFactoryUtil::store("additional-metric",
                                      AnotherMockValueType2());
    // Publish
    myPublisher.publish();
    {
        const auto fileContents =
            buildboxcommon::FileUtils::get_file_contents(metricsFilename);
        // Expect to have those two metrics
        EXPECT_NE(std::string::npos, fileContents.find("my-metric"));
        EXPECT_NE(std::string::npos, fileContents.find("additional-metric"));
    }

    // Store "another-metric"
    MetricCollectorFactoryUtil::store("another-metric",
                                      AnotherMockValueType1());
    // Publish
    myPublisher.publish();
    {
        const auto fileContents =
            buildboxcommon::FileUtils::get_file_contents(metricsFilename);
        // Expect to have those two metrics
        EXPECT_NE(std::string::npos, fileContents.find("my-metric"));
        EXPECT_NE(std::string::npos, fileContents.find("another-metric"));
        EXPECT_NE(std::string::npos, fileContents.find("additional-metric"));
    }
}
