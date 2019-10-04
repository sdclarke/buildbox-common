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
