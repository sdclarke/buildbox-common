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

#include <gtest/gtest.h>

#include <buildboxcommonmetrics_scopedperiodicpublisherdaemon.h>

using namespace buildboxcommon::buildboxcommonmetrics;

const size_t sleepSeconds = 3;
size_t numTimesPublishCalled = 0;
std::thread::id publisherThreadId;
bool publisherThreadIdIsSet = false;

struct MockPublisher {
    void publish()
    {
        if (!publisherThreadIdIsSet) {
            publisherThreadId = std::this_thread::get_id();
            publisherThreadIdIsSet = true;
        }
        numTimesPublishCalled++;
    }
};

TEST(MergeTest, ScopedPeriodicPublisherDaemon_Enabled)
{
    // reset values
    numTimesPublishCalled = 0;
    publisherThreadId = std::this_thread::get_id();

    {
        ScopedPeriodicPublisherDaemon<MockPublisher> guard(true, 1);
        std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));
    }

    EXPECT_GE(numTimesPublishCalled, sleepSeconds - 1);
    // `ScopedPeriodicPublisherDaemon.run()` will sleep 1 second before the
    // first publication.

    EXPECT_NE(std::this_thread::get_id(), publisherThreadId);
}

TEST(MergeTest, ScopedPeriodicPublisherDaemon_Disabled)
{
    // reset values
    numTimesPublishCalled = 0;

    {
        ScopedPeriodicPublisherDaemon<MockPublisher> guard(false, 1);
        std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));
    }

    EXPECT_EQ(numTimesPublishCalled, 0);
}
