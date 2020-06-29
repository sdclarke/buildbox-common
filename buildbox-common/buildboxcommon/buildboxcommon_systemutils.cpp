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

#include <buildboxcommon_systemutils.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <sstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace buildboxcommon;

namespace {
std::string logLineForCommand(const std::vector<std::string> &command)
{
    std::ostringstream command_string;
    for (const auto &token : command) {
        command_string << token << " ";
    }
    return command_string.str();
}
} // namespace

int SystemUtils::executeCommand(const std::vector<std::string> &command)
{
    const auto argc = command.size();

    // `execvp()` takes a `const char*[]`, making the conversion:
    const auto argv = std::make_unique<const char *[]>(argc + 1);
    for (unsigned int i = 0; i < argc; ++i) {
        argv[i] = command[i].c_str();
    }
    argv[argc] = nullptr;

    execv(argv[0], const_cast<char *const *>(argv.get()));
    // `execv()` does NOT search for binaries using $PATH.
    // ---------------------------------------------------------------------
    // The lines below will only be executed if `execv()` failed, otherwise
    // `execv()` does not return.

    const auto exec_error = errno;
    const auto exec_error_reason = strerror(errno);

    BUILDBOX_LOG_ERROR("Error while calling `execv("
                       << logLineForCommand(command)
                       << ")`: " << exec_error_reason);

    // Following the Bash convention for exit codes.
    // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
    if (exec_error == ENOENT) {
        return 127; // "command not found"
    }
    else {
        return 126; // Command invoked cannot execute
    }
}

std::string SystemUtils::getPathToCommand(const std::string &command)
{
    if (command.find('/') != std::string::npos) {
        return command; // `command` is a path, no need to search.
    }

    // Reading $PATH, parsing it, and looking for the binary:
    char *path_pointer = getenv("PATH");
    if (path_pointer == nullptr) {
        BUILDBOX_LOG_ERROR("Could not read $PATH");
        return "";
    }

    // `strtok()` modifies its input, making a copy:
    const std::unique_ptr<char> path_envvar_contents(strdup(path_pointer));
    if (path_envvar_contents == nullptr) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error making a copy of $PATH");
    }

    const char *separator = ":";
    char *token = strtok(path_envvar_contents.get(), separator);

    while (token != nullptr) {
        const std::string path = std::string(token) + "/" + command;

        if (FileUtils::isRegularFile(path.c_str()) &&
            FileUtils::isExecutable(path.c_str())) {
            return path;
        }

        token = strtok(nullptr, separator);
    }

    return "";
}

int SystemUtils::waitPid(const pid_t pid)
{
    while (true) {
        const int exit_code = waitPidOrSignal(pid);
        if (exit_code == -EINTR) {
            // The child can still run, keep waiting for it.
            continue;
        }
        else {
            return exit_code;
        }
    }
}

int SystemUtils::waitPidOrSignal(const pid_t pid)
{
    int status;
    const pid_t child_pid = waitpid(pid, &status, 0);

    if (child_pid == -1) { // `waitpid()` failed
        const auto waitpid_error = errno;

        if (waitpid_error == EINTR) {
            // Signal caught before child terminated.
            return -EINTR;
        }

        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error in waitpid()");
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
        // Exit code as returned by Bash.
        // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
    }

    /* According to the documentation for `waitpid(2)` we should never get
     * here:
     *
     * "If the information pointed to by stat_loc was stored by a call to
     * waitpid() that did not specify the WUNTRACED  or
     * CONTINUED flags, or by a call to the wait() function,
     * exactly one of the macros WIFEXITED(*stat_loc) and
     * WIFSIGNALED(*stat_loc) shall evaluate to a non-zero value."
     *
     * (https://pubs.opengroup.org/onlinepubs/009695399/functions/wait.html)
     */
    BUILDBOXCOMMON_THROW_EXCEPTION(
        std::runtime_error,
        "`waitpid()` returned an unexpected status: " << status);
}

std::string SystemUtils::get_current_working_directory()
{
    unsigned int bufferSize = 1024;
    while (true) {
        std::unique_ptr<char[]> buffer(new char[bufferSize]);
        char *cwd = getcwd(buffer.get(), bufferSize);

        if (cwd != nullptr) {
            return std::string(cwd);
        }
        else if (errno == ERANGE) {
            bufferSize *= 2;
        }
        else {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "current working directory not found");
        }
    }
}

void SystemUtils::redirectStandardOutputToFile(const int standardOutputFd,
                                               const std::string &path)
{
    if (standardOutputFd != STDOUT_FILENO &&
        standardOutputFd != STDERR_FILENO) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::invalid_argument,
            "File descriptor it not `STDOUT_FILENO` or `STDERR_FILENO`.");
    }

    const int fileFd =
        open(path.c_str(), O_CREAT | O_TRUNC | O_APPEND | O_WRONLY,
             S_IRUSR | S_IWUSR);

    if (fileFd == -1 || dup2(fileFd, standardOutputFd) == -1) {
        const auto outputName =
            (standardOutputFd == STDOUT_FILENO) ? "stdout" : "stderr";

        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error redirecting " << outputName << " to \"" << path << "\"");
    }
    close(fileFd);
}
