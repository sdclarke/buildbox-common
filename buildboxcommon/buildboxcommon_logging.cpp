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
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace buildboxcommon {

namespace logging {
std::string stringifyLogLevels()
{
    std::string logLevels;
    for (const auto &stringLevelPair :
         buildboxcommon::logging::logLevelToString) {
        logLevels += stringLevelPair.second + "/";
    }
    logLevels.pop_back();
    return logLevels;
}
} // namespace logging

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

// Helper function for writePrefixIfNecessary below
std::string getBasename(const std::string &fullPath)
{
    size_t startAt = std::max<size_t>(fullPath.find_last_of("/") + 1, 0);
    return fullPath.substr(startAt);
}

// 'file' purposely passed in by value
void writePrefixIfNecessary(std::ostream &os, const std::string &severity,
                            const std::string file, const int lineNumber)
{
    if (LoggerState::getInstance().isPrefixEnabled()) {
        const std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now();
        const time_t nowAsTimeT = std::chrono::system_clock::to_time_t(now);
        const std::chrono::milliseconds nowMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;
        struct tm localtime;
        localtime_r(&nowAsTimeT, &localtime);

        os << std::put_time(&localtime, "%FT%T") << '.' << std::setfill('0')
           << std::setw(3) << nowMs.count() << std::put_time(&localtime, "%z")
           << " [" << getpid() << ":" << pthread_self() << "] [ " << file
           << ":" << lineNumber << "] ";
    }

    os << "[" << severity << "] ";
}

} // namespace buildboxcommon
