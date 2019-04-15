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

#include <buildboxcommon_logging.h>

#include <iomanip>
#include <libgen.h>
#include <time.h>

namespace buildboxcommon {

LoggerState &LoggerState::getInstance()
{
    static LoggerState instance(std::clog, LogLevel::INFO);
    return instance;
}

void LoggerState::setLogLevel(LogLevel level) { d_level = level; }

void LoggerState::setLogFile(const std::string &filename)
{
    d_of.close();
    d_of.open(filename, std::ios_base::app);
    std::streambuf *buf = d_of.rdbuf();
    d_os = std::make_shared<std::ostream>(buf);
}

void LoggerState::setLogStream(std::ostream &stream)
{
    d_os = std::make_shared<std::ostream>(stream.rdbuf());
}

/* Helper function for writePrefixIfNecessary below */
std::string getBasename(const std::string &fullPath)
{
    size_t startAt = std::max<size_t>(fullPath.find_last_of("/") + 1, 0);
    return fullPath.substr(startAt);
}

void writePrefixIfNecessary(std::ostream &os, const std::string file,
                            const std::string &func)
{
    std::time_t now = std::time(nullptr);
    struct tm localtime;
    localtime_r(&now, &localtime);
    if (LoggerState::getInstance().isPrefixEnabled()) {
        os << std::put_time(&localtime, "%F %T") << " " << getBasename(file)
           << ":" << func << " ";
    }
}

} // namespace buildboxcommon
