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

#ifndef INCLUDED_BUILDBOXCOMMON_TIMEUTILS
#define INCLUDED_BUILDBOXCOMMON_TIMEUTILS

#include <chrono>
#include <google/protobuf/util/time_util.h>
#include <string>

namespace buildboxcommon {

struct TimeUtils {
    // Provide a namespace for timestamp utilities.

    /**
     * Return a Protobuf Timestamp representing the given time_point.
     */
    static const google::protobuf::Timestamp
    make_timestamp(const std::chrono::system_clock::time_point timepoint);

    /**
     * Return a timespec representing the given Protobuf Timestamp.
     */
    static const std::chrono::system_clock::time_point
    parse_timestamp(const google::protobuf::Timestamp &timestamp);

    /**
     * Return a timespec representing the given time_point.
     */
    static struct timespec
    make_timespec(const std::chrono::system_clock::time_point timepoint);

    static google::protobuf::Timestamp now();
};
} // namespace buildboxcommon

#endif
