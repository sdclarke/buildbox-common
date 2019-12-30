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

#include <stdexcept>
#include <string>

namespace buildboxcommon {

class CodePosition;

class Exception : public std::runtime_error {
  public:
    Exception(const std::string &msg, const CodePosition &cp);
};

class CodePosition {
  private:
    std::string d_file;
    int d_line;

  public:
    CodePosition(const std::string &file, const int line)
        : d_file(file), d_line(line)
    {
    }

    const std::string &file() const { return d_file; }
    int line() const { return d_line; }
};

#define BUILDBOXCOMMON_CODEPOSITION()                                         \
    buildboxcommon::CodePosition(__FILE__, __LINE__)

#define BUILDBOXCOMMON_THROW_EXCEPTION(message)                               \
    {                                                                         \
        std::ostringstream oss;                                               \
        oss << message;                                                       \
        throw buildboxcommon::Exception(oss.str(),                            \
                                        BUILDBOXCOMMON_CODEPOSITION());       \
    }

} // namespace buildboxcommon

#endif
