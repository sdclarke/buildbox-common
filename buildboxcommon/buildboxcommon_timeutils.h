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
     * Return a string representing the given time_point in the
     * ISO 8601 format (https://www.ietf.org/rfc/rfc3339.txt).
     */
    static const std::string
    make_timestamp(const std::chrono::system_clock::time_point timepoint);

    /**
     * Return a timespec representing the given ISO 8601 datetime
     * (https://www.ietf.org/rfc/rfc3339.txt).
     */
    static const std::chrono::system_clock::time_point
    parse_timestamp(const std::string &timestamp);

    /**
     * Return a timespec representing the given time_point.
     */
    static struct timespec
    make_timespec(const std::chrono::system_clock::time_point timepoint);
};
} // namespace buildboxcommon

#endif
