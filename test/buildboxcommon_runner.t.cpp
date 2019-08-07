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

#include <buildboxcommon_runner.h>

#include <buildboxcommon_temporaryfile.h>
#include <fcntl.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

class TestRunner : public Runner {
  public:
    ActionResult execute(const Command &command, const Digest &inputRoot)
    {
        return ActionResult();
    }
    using Runner::executeAndStore;
};

TEST(RunnerTest, PrintingUsageDoesntCrash)
{
    TestRunner runner;
    const char *argv[] = {"buildbox-run", nullptr};
    EXPECT_NO_THROW(runner.main(1, const_cast<char **>(argv)));
}

TEST(RunnerTest, ExecuteAndStoreHelloWorld)
{
    TestRunner runner;
    ActionResult result;

    runner.executeAndStore({"echo", "hello", "world"}, &result);
    EXPECT_EQ(result.stdout_raw(), "hello world\n");
    EXPECT_EQ(result.stderr_raw(), "");
    EXPECT_EQ(result.exit_code(), 0);
}

TEST(RunnerTest, CommandNotFound)
{
    TestRunner runner;
    ActionResult result;

    runner.executeAndStore({"command-does-not-exist"}, &result);
    EXPECT_EQ(result.exit_code(), 127); // "command not found" as in Bash
}

TEST(RunnerTest, CommandIsNotAnExecutable)
{
    TestRunner runner;
    ActionResult result;

    TemporaryFile non_executable_file;
    runner.executeAndStore({non_executable_file.name()}, &result);
    EXPECT_EQ(result.exit_code(), 126); // Command invoked cannot execute
}

TEST(RunnerTest, ExecuteAndStoreExitCode)
{
    TestRunner runner;
    ActionResult result;

    runner.executeAndStore({"sh", "-c", "exit 23"}, &result);

    EXPECT_EQ(result.stdout_raw(), "");
    EXPECT_EQ(result.stderr_raw(), "");
    EXPECT_EQ(result.exit_code(), 23);
}

TEST(RunnerTest, ExecuteAndStoreStderr)
{
    TestRunner runner;
    ActionResult result;

    runner.executeAndStore({"sh", "-c", "echo hello; echo world >&2"},
                           &result);
    EXPECT_EQ(result.stdout_raw(), "hello\n");
    EXPECT_EQ(result.stderr_raw(), "world\n");
    EXPECT_EQ(result.exit_code(), 0);
}
