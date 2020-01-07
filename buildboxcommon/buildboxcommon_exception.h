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
 * 22 int func()
 * 23 {
 * 23     const int rcode = tryThis();
 * 24     if (!rcode) {
 * 25         BUILDBOXCOMMON_THROW_EXCEPTION(std::logic_error, "tryThis failed");
 * 26     }
 * 27     return 0;
 * 28 }
 *    ...
 *      try {
 *          func();
 *      }
 *      catch (const std::logic_error &e) {
 *          std::cout << e.what() << std::endl;
 *      }
 *
 * Sample output from example #1:
 * std::logic_error exception thrown at [foo.cpp:25], errMsg = "tryThis failed"
 * ^                                     ^                      ^
 * |- Name of exception                  |- Source location     |- Descriptive text
 *
 * Example usage #2 from file buildboxcommon_client.cpp:
 * 22 void Client::upload(int fd, const Digest &digest)
 * 23 {
 * 24       const ssize_t bytesRead =
 * 25           read(fd, &buffer[0], bytestreamChunkSizeBytes());
 * 26       if (bytesRead < 0) {
 * 27           BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
 * 28               std::system_error(errno, std::generic_category()),
 * 29               "Error in read on descriptor " << fd);
 * 30       }
 * 31 }
 *    ...
 *      try {
 *          client.upload(-40, digest);
 *      }
 *      catch (const std::system_error &e) {
 *          std::cout << e.what() << std::endl;
 *      }
 *
 * Sample output from example #2:
 * exception thrown at [buildboxcommon_client.cpp:29] [generic:9], errMsg = "Error in read on descriptor -40", errno : Bad file descriptor
 *                      ^                              ^                    ^                                          ^
 *                      |- Source location             |- Category/errno    |- Descriptive text                        |- Errno text
 */
// clang-format on

struct ExceptionUtil {
    static std::string basename(const std::string &fileName);
};

#define BUILDBOXCOMMON_THROW_EXCEPTION(exception, what)                       \
    {                                                                         \
        std::ostringstream oss;                                               \
        oss << #exception << " exception thrown at "                          \
            << "[" << ExceptionUtil::basename(__FILE__) << ":" << __LINE__    \
            << "], errMsg = \"" << what << "\"";                              \
        throw exception(oss.str());                                           \
    }

#define BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(exception, what)                \
    {                                                                         \
        std::ostringstream oss;                                               \
        oss << "exception thrown at "                                         \
            << "[" << ExceptionUtil::basename(__FILE__) << ":" << __LINE__    \
            << "] [" << exception.code().category().name() << ":"             \
            << exception.code().value() << "], errMsg = \"" << what           \
            << "\", errno ";                                                  \
        throw std::system_error(exception.code(), oss.str());                 \
    }

} // namespace buildboxcommon

#endif
