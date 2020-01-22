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

#include <buildboxcommon_timeutils.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_logging.h>

#include <system_error>

namespace buildboxcommon {

const std::string
TimeUtils::make_timestamp(const std::chrono::system_clock::time_point mtime)
{
    auto usec = (google::protobuf::int64)
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        mtime.time_since_epoch())
                        .count();
    auto time =
        google::protobuf::util::TimeUtil::MicrosecondsToTimestamp(usec);
    const std::string timestamp =
        google::protobuf::util::TimeUtil::ToString(time);

    return timestamp;
}

const std::chrono::system_clock::time_point
TimeUtils::parse_timestamp(const std::string &timestamp)
{
    google::protobuf::Timestamp gtime;
    if (google::protobuf::util::TimeUtil::FromString(timestamp, &gtime)) {
        const std::chrono::system_clock::time_point timepoint =
            std::chrono::system_clock::from_time_t(
                static_cast<time_t>(gtime.seconds())) +
            std::chrono::microseconds{static_cast<long>(gtime.nanos()) / 1000};

        return timepoint;
    }
    else {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Failed to parse timestamp: \""
                                           << timestamp << "\"");
    }
}

struct timespec
TimeUtils::make_timespec(std::chrono::system_clock::time_point timepoint)
{
    struct timespec time;
    const int micro = 1000000;
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
        timepoint.time_since_epoch());
    time.tv_sec = static_cast<time_t>(usec.count() / micro);
    time.tv_nsec = static_cast<long>((usec.count() % micro) * 1000);
    return time;
}

} // namespace buildboxcommon
