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
#include <buildboxcommon_temporaryfile.h>
#include <gtest/gtest.h>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

TEST(SystemUtilsTests, CommandNotFound)
{
    ASSERT_EQ(buildboxcommon::SystemUtils::executeCommand(
                  {"command-does-not-exist"}),
              127); // 127 == "command not found" as in Bash
}

TEST(SystemUtilsTests, CommandIsNotAnExecutable)
{
    TemporaryFile non_executable_file;

    ASSERT_EQ(buildboxcommon::SystemUtils::executeCommand(
                  {non_executable_file.name()}),
              126); // 126 == Command invoked cannot execute
}

TEST(SystemUtilsTests, WaitPidExitCode)
{
    // Creating a subprocess:
    const int pid = fork();
    ASSERT_NE(pid, -1);

    // The subprocess exits:
    if (pid == 0) {
        exit(42);
    }
    // And the parent gets its exit code:
    else {
        const int exit_status = SystemUtils::waitPid(pid);
        ASSERT_EQ(exit_status, 42);
    }
}

TEST(SystemUtilsTests, WaitPidSignalNumber)
{
    // Creating a subprocess:
    const int pid = fork();
    ASSERT_NE(pid, -1);

    const auto signal_number = SIGKILL;
    // The subprocess gets signaled:
    if (pid == 0) {
        raise(signal_number);
    }
    // And the parent gets an exit code that encodes the signal number as done
    // by Bash:
    else {
        const int exit_status = SystemUtils::waitPid(pid);
        ASSERT_EQ(exit_status, signal_number + 128);
    }
}

TEST(SystemUtilsTests, WaitPidThrowsOnError)
{
    const int invalid_pid = -1;
    ASSERT_THROW(SystemUtils::waitPid(invalid_pid), std::system_error);
}
