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

    execvp(argv[0], const_cast<char *const *>(argv.get()));
    // ---------------------------------------------------------------------
    // The lines below will only be executed if `execvp()` failed, otherwise
    // `execvp()` does not return.

    const auto exec_error = errno;
    const auto exec_error_reason = strerror(errno);

    BUILDBOX_LOG_ERROR("Error while calling `execvp("
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

int SystemUtils::waitPid(const pid_t pid)
{
    while (true) {
        int status;
        const pid_t child_pid = waitpid(pid, &status, 0);

        if (child_pid == -1) { // `waitpid()` failed
            const auto waitpid_error = errno;

            if (waitpid_error == EINTR) {
                // The child can still run, keep waiting for it.
                continue;
            }

            throw std::system_error(waitpid_error, std::system_category());
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
        throw std::runtime_error(
            "`waitpid()` returned an unexpected status: " +
            std::to_string(status));
    }
}
