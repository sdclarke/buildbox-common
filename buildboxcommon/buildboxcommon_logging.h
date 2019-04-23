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

#ifndef INCLUDED_BUILDBOXCOMMON_LOGGING
#define INCLUDED_BUILDBOXCOMMON_LOGGING

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace buildboxcommon {

enum LogLevel { DEBUG = 0, INFO = 1, ERROR = 2 };

// TODO: Expand this namespace to encompass the whole file
// Will require lots of MRs in dependent projects
namespace logging {

const std::unordered_map<std::string, LogLevel> stringToLogLevel = {
    {"debug", LogLevel::DEBUG},
    {"info", LogLevel::INFO},
    {"error", LogLevel::ERROR}};

const std::unordered_map<LogLevel, std::string, std::hash<int>>
    logLevelToString = {{LogLevel::DEBUG, "debug"},
                        {LogLevel::INFO, "info"},
                        {LogLevel::ERROR, "error"}};

} // namespace logging

/**
 * Singleton class that maintains the logging state for the various logger
 * macros defined below. State includes the output stream, log level, and
 * whether to output the log prefix.
 */
class LoggerState {
  private:
    LoggerState();
    LoggerState(std::ostream &os, LogLevel level) : d_level(level)
    {
        d_os = std::make_shared<std::ostream>(os.rdbuf());
    }

    std::shared_ptr<std::ostream> d_os;

    // We need an ofstream member to support setLogFile().
    std::ofstream d_of;
    LogLevel d_level;
    bool d_prefixEnabled = true;
    std::stringstream d_stringstream;

  public:
    static LoggerState &getInstance();

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() { return d_level; }

    void setLogFile(const std::string &filename);
    void setLogStream(std::ostream &stream);
    std::shared_ptr<std::ostream> getOutputStream() { return d_os; }

    void enablePrefix() { d_prefixEnabled = true; }
    void disablePrefix() { d_prefixEnabled = false; }
    bool isPrefixEnabled() { return d_prefixEnabled; }
};

void writePrefixIfNecessary(std::ostream &ss, const std::string file,
                            const std::string &func);

} // namespace buildboxcommon

#define BUILDBOX_LOG_SET_LEVEL(logLevel)                                      \
    buildboxcommon::LoggerState::getInstance().setLogLevel(logLevel);

#define BUILDBOX_LOG_SET_FILE(filename)                                       \
    buildboxcommon::LoggerState::getInstance().setLogFile(filename);

#define BUILDBOX_LOG_SET_STREAM(stream)                                       \
    buildboxcommon::LoggerState::getInstance().setLogStream(stream);

#define BUILDBOX_LOG_ENABLE_PREFIX()                                          \
    buildboxcommon::LoggerState::getInstance().enablePrefix();

#define BUILDBOX_LOG_DISABLE_PREFIX()                                         \
    buildboxcommon::LoggerState::getInstance().disablePrefix();

/*
 * These logger macros work by formatting the logline with a stringstream,
 * then passing it to the appropriate logger function. The prefix is added to
 * the stringstream before the logline if enabled.
 */

#define BUILDBOX_LOG_DEBUG(x)                                                 \
    {                                                                         \
        if (buildboxcommon::LoggerState::getInstance().getLogLevel() <=       \
            buildboxcommon::LogLevel::DEBUG) {                                \
            auto _bb_log_os =                                                 \
                buildboxcommon::LoggerState::getInstance().getOutputStream(); \
            *_bb_log_os << "[DEBUG] ";                                        \
            buildboxcommon::writePrefixIfNecessary(*_bb_log_os, __FILE__,     \
                                                   __func__);                 \
            *_bb_log_os << x << std::endl << std::flush;                      \
        }                                                                     \
    }

#define BUILDBOX_LOG_INFO(x)                                                  \
    {                                                                         \
        if (buildboxcommon::LoggerState::getInstance().getLogLevel() <=       \
            buildboxcommon::LogLevel::INFO) {                                 \
            auto _bb_log_os =                                                 \
                buildboxcommon::LoggerState::getInstance().getOutputStream(); \
            *_bb_log_os << "[INFO] ";                                         \
            buildboxcommon::writePrefixIfNecessary(*_bb_log_os, __FILE__,     \
                                                   __func__);                 \
            *_bb_log_os << x << std::endl << std::flush;                      \
        }                                                                     \
    }

#define BUILDBOX_LOG_ERROR(x)                                                 \
    {                                                                         \
        if (buildboxcommon::LoggerState::getInstance().getLogLevel() <=       \
            buildboxcommon::LogLevel::ERROR) {                                \
            auto _bb_log_os =                                                 \
                buildboxcommon::LoggerState::getInstance().getOutputStream(); \
            *_bb_log_os << "[ERROR] ";                                        \
            buildboxcommon::writePrefixIfNecessary(*_bb_log_os, __FILE__,     \
                                                   __func__);                 \
            *_bb_log_os << x << std::endl << std::flush;                      \
        }                                                                     \
    }

#endif
