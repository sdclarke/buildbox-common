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

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace buildboxcommon {

namespace logging {
Logger::Logger() : d_logOutputDirectory(nullptr), d_glogInitialized(false) {}

Logger &Logger::getLoggerInstance()
{
    static Logger logger;
    return logger;
}

void Logger::setOutputDirectory(const char *outputDirectory)
{
    if (d_glogInitialized) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Output directories must be specified "
                                "before Logger instance is initialized.");
    }

    d_logOutputDirectory = outputDirectory;
}

void Logger::initialize(const char *programName)
{
    if (programName == nullptr || strlen(programName) == 0) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Initialize() must be called with a non-empty program name");
    }

    if (d_glogInitialized) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Attempted to initialize Logger instance more than once.");
    }

    if (d_logOutputDirectory) {
        FLAGS_log_dir = d_logOutputDirectory;
        FLAGS_logtostderr = false;
        FLAGS_alsologtostderr = false;
    }
    else {
        FLAGS_logtostderr = true;
    }

    // Since we will add our own, we also disable glog's own prefix using
    // the following global variable.
    FLAGS_log_prefix = false;

    google::InitGoogleLogging(programName);
    d_glogInitialized = true;
}

void Logger::setLogLevel(LogLevel logLevel)
{
    if (FLAGS_logtostderr || FLAGS_alsologtostderr) {
        google::LogSeverity glogMinSeverity;
        if (logLevel == buildboxcommon::LogLevel::WARNING) {
            glogMinSeverity = google::GLOG_WARNING;
        }
        else if (logLevel == buildboxcommon::LogLevel::ERROR) {
            glogMinSeverity = google::GLOG_ERROR;
        }
        else {
            /* By default show all messages */
            glogMinSeverity = google::GLOG_INFO;
        }

        google::SetStderrLogging(glogMinSeverity);

        /* For the verbose levels, we also set the VLOG max. level */
        if (logLevel == LogLevel::DEBUG) {
            google::SetVLOGLevel("*", GlogVlogLevels::GLOG_VLOG_LEVEL_DEBUG);
        }
        else if (logLevel == LogLevel::TRACE) {
            google::SetVLOGLevel("*", GlogVlogLevels::GLOG_VLOG_LEVEL_TRACE);
        }
    }
}

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

std::string printableCommandLine(const std::vector<std::string> &commandLine)
{
    if (commandLine.empty()) {
        return "";
    }
    std::ostringstream commandLineStream;
    // -1, to avoid putting space at end of string
    copy(commandLine.begin(), commandLine.end() - 1,
         std::ostream_iterator<std::string>(commandLineStream, " "));
    commandLineStream << commandLine.back();
    return commandLineStream.str();
}

std::string logPrefix(const std::string &severity, const std::string file,
                      const int lineNumber)
{
    std::ostringstream os;
    writeLogPrefix(severity, file, lineNumber, os);
    return os.str();
}

std::ostream &writeLogPrefix(const std::string &severity,
                             const std::string &file, const int lineNumber,
                             std::ostream &os)
{
    const std::chrono::system_clock::time_point now =
        std::chrono::system_clock::now();
    const time_t nowAsTimeT = std::chrono::system_clock::to_time_t(now);
    const std::chrono::milliseconds nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) %
        1000;
    struct tm localtime;
    localtime_r(&nowAsTimeT, &localtime);

    const auto basenameStart = std::max<size_t>(file.find_last_of("/") + 1, 0);

    os << std::put_time(&localtime, "%FT%T") << '.' << std::setfill('0')
       << std::setw(3) << nowMs.count() << std::put_time(&localtime, "%z")
       << " [" << getpid() << ":" << pthread_self() << "] ["
       << file.substr(basenameStart) << ":" << lineNumber << "] ";
    os << "[" << severity << "] ";

    return os;
}

} // namespace logging

} // namespace buildboxcommon
