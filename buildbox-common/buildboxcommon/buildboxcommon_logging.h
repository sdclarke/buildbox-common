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

#include <buildboxcommon_exception.h>

#pragma GCC diagnostic ignored "-Wsuggest-override"
#include <glog/logging.h>
#pragma GCC diagnostic pop

#include <map>
#include <string>

namespace {
// glog supports these standard levels: FATAL, ERROR, WARNING, and INFO.
// FATAL terminates the program (!)

// For `LogLevel::DEBUG` and `LogLevel::TRACE` we will rely on glog's verbose
// logging.
// That allows assigning arbitrary levels (int values) for messages that should
// be displayed only in verbose mode. Users can then specify a limit with the
// `--v=L` option so that they see messages from levels that do not exceed
// `L`.

enum GlogVlogLevels { GLOG_VLOG_LEVEL_DEBUG = 1, GLOG_VLOG_LEVEL_TRACE = 2 };

// For example: if `--v==2`, `TRACE` messages will be shown but not if
// `--v==1`.
//
// This is an implementation detail, so we hide those values.
// Users of `buildboxcommon_logging.h` will always refer to
// `buildboxcommon::LogLevel` values.
} // namespace

namespace buildboxcommon {

enum LogLevel { TRACE = 0, DEBUG = 1, INFO = 2, WARNING = 3, ERROR = 4 };

// TODO: Expand this namespace to encompass the whole file
// Will require lots of MRs in dependent projects
namespace logging {

class Logger {
    /*
     * This singleton class helps configure and initialize glog.
     *
     * Example usage:
     *
     * ```
     *  int main(int argc, char *argv[]) {
     *    auto &logger = buildboxcommon::logging::Logger::getLoggerInstance();
     *
     *    if (argv contains option to write logs to files) {
     *      // Redirecting outputs *before* initializing logger:
     *      logger.setOutputDirectory(outputDirectory);
     *    }
     *
     *    logger.initialize(argv[0]); // Must be always called.
     *
     *    BUILDBOX_LOG_SET_LEVEL(buildboxcommon::LogLevel::TRACE);
     *    // (Levels can be changed after initialization)
     * }
     * ```
     */
  public:
    static Logger &getLoggerInstance();

    // Write logs to files in the given directory (ERROR messages
    // will still be printed to stderr).
    // Files will be named "<programName>.<hostname>.<user name>.log.<severity
    // level>.<date>.<time>.<pid>".
    // This method must be called before initializing the Logger instance, so
    // the destination cannot be changed again.
    void setOutputDirectory(const char *outputDirectory);

    // Set the necessary configuration and initialize glog. Must be called only
    // once, before starting to write log messages. (Program name should
    // generally be `argv[0]`.)
    void initialize(const char *programName);

    // Set the maximum log level of messages that will be shown in stderr.
    // (This function can be called after initialization.)
    void setLogLevel(LogLevel logLevel);

    Logger(const Logger &) = delete;
    Logger &operator=(Logger const &) = delete;

  private:
    Logger();

    // Optional directory to which log files are written to.
    const char *d_logOutputDirectory;

    // Keeps track of whether `initialize()` has been called.
    bool d_glogInitialized;

}; // namespace logging

const std::map<std::string, LogLevel> stringToLogLevel = {
    {"trace", LogLevel::TRACE},
    {"debug", LogLevel::DEBUG},
    {"info", LogLevel::INFO},
    {"warning", LogLevel::WARNING},
    {"error", LogLevel::ERROR}};

const std::map<LogLevel, std::string> logLevelToString = {
    {LogLevel::TRACE, "trace"},
    {LogLevel::DEBUG, "debug"},
    {LogLevel::INFO, "info"},
    {LogLevel::WARNING, "warning"},
    {LogLevel::ERROR, "error"}};

std::string stringifyLogLevels();

std::string printableCommandLine(const std::vector<std::string> &commandLine);

// This function generates the prefix that gets attached to every log line.
std::string logPrefix(const std::string &severity, const std::string file,
                      const int lineNumber);

// Write the log prefix to the given ostream and return a reference to it to
// allow chaining.
std::ostream &writeLogPrefix(const std::string &severity,
                             const std::string &file, const int lineNumber,
                             std::ostream &os);

} // namespace logging
} // namespace buildboxcommon

// This sets the minimum level what will be logged to stderr.
// By default we use `INFO`.
#define BUILDBOX_LOG_SET_LEVEL(logLevel)                                      \
    buildboxcommon::logging::Logger::getLoggerInstance().setLogLevel(logLevel);

#define BUILDBOX_LOG_TRACE(x)                                                 \
    {                                                                         \
        VLOG(GlogVlogLevels::GLOG_VLOG_LEVEL_TRACE)                           \
            << buildboxcommon::logging::logPrefix("TRACE", __FILE__,          \
                                                  __LINE__)                   \
            << x;                                                             \
    }

#define BUILDBOX_LOG_DEBUG(x)                                                 \
    {                                                                         \
        VLOG(GlogVlogLevels::GLOG_VLOG_LEVEL_DEBUG)                           \
            << buildboxcommon::logging::logPrefix("DEBUG", __FILE__,          \
                                                  __LINE__)                   \
            << x;                                                             \
    }

#define BUILDBOX_LOG_INFO(x)                                                  \
    {                                                                         \
        buildboxcommon::logging::writeLogPrefix("INFO", __FILE__, __LINE__,   \
                                                LOG(INFO))                    \
            << x;                                                             \
    }

#define BUILDBOX_LOG_WARNING(x)                                               \
    {                                                                         \
        buildboxcommon::logging::writeLogPrefix("WARNING", __FILE__,          \
                                                __LINE__, LOG(WARNING))       \
            << x;                                                             \
    }

#define BUILDBOX_LOG_ERROR(x)                                                 \
    {                                                                         \
        buildboxcommon::logging::writeLogPrefix("ERROR", __FILE__, __LINE__,  \
                                                LOG(ERROR))                   \
            << x;                                                             \
    }

#endif
