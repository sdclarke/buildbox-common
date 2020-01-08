/*
 * Copyright 2018 Bloomberg Finance LP
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

#ifndef INCLUDED_BUILDBOXCOMMON_EXCEPTION
#define INCLUDED_BUILDBOXCOMMON_EXCEPTION

#include <libgen.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace buildboxcommon {

/*
 * Use these macros to programatically capture the source location(file, line)
 * of a throw'd exception
 */

// clang-format off
/*
 * Example usage #1 from file foo.cpp:
 * int func()
 * {
 *     const int rcode = tryThis();
 *     if (!rcode) {
 *         BUILDBOXCOMMON_THROW_EXCEPTION(std::logic_error, "tryThis failed");
 *     }
 *     return 0;
 * }
 * ...
 *     try {
 *         func();
 *     }
 *     catch (const std::logic_error &e) {
 *         std::cout << e.what() << std::endl;
 *     }
 *
 * Sample output from example #1:
 * std::logic_error exception thrown at [foo.cpp:25], errMsg = "tryThis failed"
 * ^                                     ^                      ^
 * |- Name of exception                  |- Source location     |- Descriptive text
 *
 * Example usage #2 from file buildboxcommon_client.cpp:
 * void Client::upload(int fd, const Digest &digest)
 * {
 *     const ssize_t bytesRead =
 *         read(fd, &buffer[0], bytestreamChunkSizeBytes());
 *     if (bytesRead < 0) {
 *         BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
 *             std::system_error(errno, std::generic_category()),
 *             "Error in read on descriptor " << fd);
 *     }
 * }
 * ...
 *     try {
 *         client.upload(-40, digest);
 *     }
 *     catch (const std::system_error &e) {
 *         std::cout << e.what() << std::endl;
 *     }
 *
 * Sample output from example #2:
 * exception thrown at [buildboxcommon_client.cpp:29] [generic:9], errMsg = "Error in read on descriptor -40", errno : Bad file descriptor
 *                      ^                              ^                    ^                                          ^
 *                      |- Source location             |- Category/errno    |- Descriptive text                        |- Errno text
 */
// clang-format on

#define BUILDBOXCOMMON_THROW_EXCEPTION(exception, what)                       \
    {                                                                         \
        char ___tmp_file_name[] = {__FILE__};                                 \
        std::ostringstream __what__stream;                                    \
        __what__stream << #exception << " exception thrown at "               \
                       << "[" << ::basename(___tmp_file_name) << ":"          \
                       << __LINE__ << "], errMsg = \"" << what << "\"";       \
        throw exception(__what__stream.str());                                \
    }

#define BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(exception, what)                \
    {                                                                         \
        char ___tmp_file_name[] = {__FILE__};                                 \
        std::ostringstream __what__stream;                                    \
        __what__stream << "exception thrown at "                              \
                       << "[" << ::basename(___tmp_file_name) << ":"          \
                       << __LINE__ << "] ["                                   \
                       << exception.code().category().name() << ":"           \
                       << exception.code().value() << "], errMsg = \""        \
                       << what << "\", errno ";                               \
        throw std::system_error(exception.code(), __what__stream.str());      \
    }

} // namespace buildboxcommon

#endif
