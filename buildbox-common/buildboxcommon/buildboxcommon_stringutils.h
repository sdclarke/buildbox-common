/*
 * Copyright 2020 Bloomberg Finance LP
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

#ifndef INCLUDED_BUILDBOXCOMMON_STRINGUTILS
#define INCLUDED_BUILDBOXCOMMON_STRINGUTILS

#include <functional>
#include <string>

namespace buildboxcommon {

struct StringUtils {

    // all the single parameter methods below default to using std::isspace
    // semantics
    static void ltrim(std::string *s);
    static void ltrim(std::string *s, const std::function<int(int)> &filter);
    static void rtrim(std::string *s);
    static void rtrim(std::string *s, const std::function<int(int)> &filter);
    static void trim(std::string *s);
    static void trim(std::string *s, const std::function<int(int)> &filter);

    // return copies
    static std::string ltrim(const std::string &s);
    static std::string ltrim(const std::string &s,
                             const std::function<int(int)> &filter);
    static std::string rtrim(const std::string &s);
    static std::string rtrim(const std::string &s,
                             const std::function<int(int)> &filter);
    static std::string trim(const std::string &s);
    static std::string trim(const std::string &s,
                            const std::function<int(int)> &filter);
};

} // namespace buildboxcommon

#endif
