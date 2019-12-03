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

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_systemutils.h>
#include <buildboxcommon_temporarydirectory.h>
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
    // expose createOutputDirectories for testing
    void testCreateOutputDirectories(const Command &command,
                                     const std::string &workingDir) const
    {
        createOutputDirectories(command, workingDir);
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

TEST(RunnerTest, CreateOutputDirectoriesTest)
{
    TestRunner runner;
    const std::string cwd = SystemUtils::get_current_working_directory();
    std::vector<std::string> output_directories{"build_t/intermediate",
                                                "tmp_t/build", "empty", ""};
    std::vector<std::string> output_files{
        "intermediate_t/tmp.o", "artifacts_t/build.o", "empty.txt", ""};

    std::vector<std::string> expected_directories{
        "build_t", "tmp_t", "intermediate_t", "artifacts_t"};

    for (const auto &dir : expected_directories) {
        std::string full_path = cwd + "/" + dir;
        // directories should not exist
        EXPECT_FALSE(FileUtils::is_directory(full_path.c_str()));
    }

    Command command;
    for (const auto &dir : output_directories) {
        *command.add_output_directories() = dir;
    }

    for (const auto &dir : output_files) {
        *command.add_output_files() = dir;
    }

    runner.testCreateOutputDirectories(command, cwd);

    for (const auto &dir : expected_directories) {
        std::string full_path = cwd + "/" + dir;
        // directories should now exist
        EXPECT_TRUE(FileUtils::is_directory(full_path.c_str()));
        // clean up directory now
        FileUtils::delete_directory(full_path.c_str());
    }
}

TEST(RunnerTest, ChmodDirectory)
{
    TemporaryDirectory dir;
    const std::string subdirectory_path = std::string(dir.name()) + "/subdir";

    mode_t perm = 0555;
    // create subdirectory with restrictive permissions.
    FileUtils::create_directory(subdirectory_path.c_str(), perm);

    // check permissions of subdirectory
    struct stat sb;
    stat(subdirectory_path.c_str(), &sb);
    ASSERT_EQ((unsigned long)sb.st_mode & 0777, perm);

    // change permissions of directory
    perm = 0777;
    Runner::recursively_chmod_directories(dir.name(), perm);

    // check permissions of top level, and sub directories.
    stat(dir.name(), &sb);
    ASSERT_EQ((unsigned long)sb.st_mode & 0777, perm);

    stat(subdirectory_path.c_str(), &sb);
    ASSERT_EQ((unsigned long)sb.st_mode & 0777, perm);
}
