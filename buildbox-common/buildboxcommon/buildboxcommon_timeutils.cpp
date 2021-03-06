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

const google::protobuf::Timestamp
TimeUtils::make_timestamp(const std::chrono::system_clock::time_point mtime)
{
    auto usec = (google::protobuf::int64)
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        mtime.time_since_epoch())
                        .count();
    return google::protobuf::util::TimeUtil::MicrosecondsToTimestamp(usec);
}

const std::chrono::system_clock::time_point
TimeUtils::parse_timestamp(const google::protobuf::Timestamp &timestamp)
{
    const std::chrono::system_clock::time_point timepoint =
        std::chrono::system_clock::from_time_t(
            static_cast<time_t>(timestamp.seconds())) +
        std::chrono::microseconds{static_cast<long>(timestamp.nanos()) / 1000};

    return timepoint;
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

google::protobuf::Timestamp TimeUtils::now()
{
    google::protobuf::Timestamp res;
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) != 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Could not read current time.")
    }

    res.set_seconds(tv.tv_sec);
    res.set_nanos(static_cast<google::protobuf::int32>(tv.tv_usec * 1000));
    return res;
}

} // namespace buildboxcommon
