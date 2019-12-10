/*
 * Copyright 2019 Bloomberg Finance LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "buildboxcommontest_utils.h"
#include <buildboxcommon_timeutils.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(TimeUtilsTests, MakeTimestamp)
{
    // translate std::chrono::system_clock::time_point into
    // an ISO 8601 std::string timestamp
    std::chrono::microseconds usec((long)1575452181012345);
    ASSERT_EQ(usec.count(), (long)1575452181012345);
    std::string expected = "2019-12-04T09:36:21.012345Z";
    std::chrono::system_clock::time_point mtime(usec);
    std::string timestamp = TimeUtils::make_timestamp(mtime);
    ASSERT_EQ(timestamp, expected);
}

TEST(TimeUtilsTests, ParseTimestamp)
{
    // translate an ISO 8601 std::string timestamp into a
    // std::chrono::system_clock::time_point
    std::string timestamp = "2019-12-04T09:36:21.012345Z";
    std::chrono::microseconds exp((long)1575452181012345);
    std::chrono::system_clock::time_point timepoint =
        TimeUtils::parse_timestamp(timestamp);
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
        timepoint.time_since_epoch());
    ASSERT_EQ(exp.count(), usec.count());
}

TEST(TimeUtilsTests, MakeTimespec)
{
    // translate a std::chrono::system_clock::time_point into a struct timespec
    const std::chrono::microseconds usec((long)1575452181012345);
    const std::chrono::system_clock::time_point timepoint(usec);
    struct timespec mtime = TimeUtils::make_timespec(timepoint);
    ASSERT_EQ(mtime.tv_sec, 1575452181);
    ASSERT_EQ(mtime.tv_nsec, 12345000);
}
