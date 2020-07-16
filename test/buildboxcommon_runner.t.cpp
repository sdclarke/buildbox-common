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
    ActionResult execute(const Command &, const Digest &)
    {
        return ActionResult();
    }
    // expose createOutputDirectories for testing
    void testCreateOutputDirectories(const Command &command,
                                     const std::string &workingDir) const
    {
        createOutputDirectories(command, workingDir);
    }

    static std::pair<Digest, Digest>
    dummyUploadFunction(const std::string &stdout_file,
                        const std::string &stderr_file)
    {
        // Return valid hashes so that tests can verify that this was invoked
        // with the expected data:
        return std::make_pair(CASHash::hashFile(stdout_file),
                              CASHash::hashFile(stderr_file));
    }

    void setStdOutFile(const std::string &path)
    {
        this->d_standardOutputsCaptureConfig.stdout_file_path = path;
    }

    void setStdErrFile(const std::string &path)
    {
        this->d_standardOutputsCaptureConfig.stderr_file_path = path;
    }

    void skipStandardOutputCapture()
    {
        this->d_standardOutputsCaptureConfig.skip_capture = true;
    }

    using Runner::executeAndStore;
};

TEST(RunnerTest, PrintingUsageDoesntCrash)
{
    TestRunner runner;
    const char *argv[] = {"buildbox-run", nullptr};
    EXPECT_NO_THROW(runner.main(1, const_cast<char **>(argv)));
}

void assert_metadata_execution_timestamps_set(const ActionResult &result)
{
    // `ExecutedActionMetadata` execution timestamps are set:
    const auto empty_timestamp = google::protobuf::Timestamp();
    EXPECT_NE(result.execution_metadata().execution_start_timestamp(),
              empty_timestamp);
    EXPECT_NE(result.execution_metadata().execution_completed_timestamp(),
              empty_timestamp);

    // But the remaining timestamps aren't modified:
    EXPECT_EQ(result.execution_metadata().worker_start_timestamp(),
              empty_timestamp);
    EXPECT_EQ(result.execution_metadata().worker_completed_timestamp(),
              empty_timestamp);
    EXPECT_EQ(result.execution_metadata().worker_start_timestamp(),
              result.execution_metadata().worker_start_timestamp());
    EXPECT_EQ(result.execution_metadata().worker_completed_timestamp(),
              result.execution_metadata().worker_start_timestamp());
}

class TestOutputCaptureFixture : public ::testing::TestWithParam<bool> {

  protected:
    TestOutputCaptureFixture(){};

    TestRunner test_runner;
};

TEST_P(TestOutputCaptureFixture, ExecuteAndStoreHelloWorld)
{
    ActionResult result;

    std::unique_ptr<buildboxcommon::TemporaryFile> stdout_file;
    std::unique_ptr<buildboxcommon::TemporaryFile> stderr_file;
    const auto standard_outputs_redirected_to_custom_paths = GetParam();
    if (standard_outputs_redirected_to_custom_paths) {
        // We redirect the command's standard outputs to specific files.
        // Otherwise the runner will create and use its own temporay ones.
        stdout_file = std::make_unique<buildboxcommon::TemporaryFile>();
        stderr_file = std::make_unique<buildboxcommon::TemporaryFile>();

        test_runner.setStdOutFile(stdout_file->strname());
        test_runner.setStdErrFile(stderr_file->strname());
    }

    const auto path_to_echo = SystemUtils::getPathToCommand("echo");
    ASSERT_FALSE(path_to_echo.empty());
    test_runner.executeAndStore({path_to_echo, "hello", "world"},
                                TestRunner::dummyUploadFunction, &result);

    const auto expected_stdout = "hello world\n";
    EXPECT_EQ(result.stdout_digest(), CASHash::hash(expected_stdout));
    EXPECT_TRUE(result.stdout_raw().empty()); // `Runner` does not inline.

    EXPECT_EQ(result.stderr_digest(), CASHash::hash(""));

    EXPECT_EQ(result.exit_code(), 0);

    assert_metadata_execution_timestamps_set(result);

    if (standard_outputs_redirected_to_custom_paths) {
        EXPECT_EQ(FileUtils::getFileContents(stdout_file->name()),
                  expected_stdout);
        EXPECT_EQ(FileUtils::getFileContents(stderr_file->name()), "");
    }
}
INSTANTIATE_TEST_CASE_P(OutputRedirectionTest, TestOutputCaptureFixture,
                        ::testing::Values(false, true));

TEST(RunnerTest, TestEmptyOutputsNotUploaded)
{
    TestRunner runner;
    ActionResult result;

    const auto path_to_true = SystemUtils::getPathToCommand("true");
    ASSERT_FALSE(path_to_true.empty());

    runner.executeAndStore({path_to_true}, TestRunner::dummyUploadFunction,
                           &result);

    EXPECT_EQ(result.stdout_digest(), CASHash::hash(""));
    EXPECT_EQ(result.stderr_digest(), CASHash::hash(""));
    EXPECT_EQ(result.exit_code(), 0);

    assert_metadata_execution_timestamps_set(result);
}

TEST(RunnerTest, CommandNotFound)
{
    TestRunner runner;
    ActionResult result;

    runner.executeAndStore({"command-does-not-exist"},
                           TestRunner::dummyUploadFunction, &result);
    EXPECT_EQ(result.exit_code(), 127); // "command not found" as in Bash

    assert_metadata_execution_timestamps_set(result);
}

TEST(RunnerTest, CommandIsNotAnExecutable)
{
    TestRunner runner;
    ActionResult result;

    TemporaryFile non_executable_file;
    runner.executeAndStore({non_executable_file.name()},
                           TestRunner::dummyUploadFunction, &result);
    EXPECT_EQ(result.exit_code(), 126); // Command invoked cannot execute

    assert_metadata_execution_timestamps_set(result);
}

TEST(RunnerTest, ExecuteAndStoreExitCode)
{
    TestRunner runner;
    ActionResult result;

    const auto path_to_sh = SystemUtils::getPathToCommand("sh");
    ASSERT_FALSE(path_to_sh.empty());

    runner.executeAndStore({path_to_sh, "-c", "exit 23"},
                           TestRunner::dummyUploadFunction, &result);

    EXPECT_EQ(result.exit_code(), 23);
}

TEST(RunnerTest, ExecuteAndStoreStderr)
{
    TestRunner runner;
    ActionResult result;

    const auto path_to_sh = SystemUtils::getPathToCommand("sh");
    ASSERT_FALSE(path_to_sh.empty());

    runner.executeAndStore({path_to_sh, "-c", "echo hello; echo world >&2"},
                           TestRunner::dummyUploadFunction, &result);

    const auto expected_stdout = "hello\n";
    const auto expected_stderr = "world\n";

    EXPECT_EQ(result.stdout_digest(), CASHash::hash(expected_stdout));
    EXPECT_EQ(result.stderr_digest(), CASHash::hash(expected_stderr));

    EXPECT_TRUE(result.stdout_raw().empty());
    EXPECT_TRUE(result.stderr_raw().empty());

    EXPECT_EQ(result.exit_code(), 0);
}

TEST(RunnerTest, ExecuteAndStoreWithoutStandardOutputCapture)
{
    const auto path_to_false = SystemUtils::getPathToCommand("false");
    ASSERT_FALSE(path_to_false.empty());

    TestRunner runner;
    runner.skipStandardOutputCapture();
    // Checking that this callback is never invoked:
    bool callback_invoked = false;
    const auto upload_callback = [&callback_invoked](const std::string &,
                                                     const std::string &) {
        callback_invoked = true;
        return std::make_pair(Digest(), Digest());
    };

    ActionResult result;
    runner.executeAndStore({path_to_false}, upload_callback, &result);
    ASSERT_FALSE(callback_invoked);

    EXPECT_FALSE(result.has_stdout_digest());
    EXPECT_FALSE(result.has_stderr_digest());
    EXPECT_NE(result.exit_code(), 0);

    assert_metadata_execution_timestamps_set(result);
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
        EXPECT_FALSE(FileUtils::isDirectory(full_path.c_str()));
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
        EXPECT_TRUE(FileUtils::isDirectory(full_path.c_str()));
        // clean up directory now
        FileUtils::deleteDirectory(full_path.c_str());
    }
}

TEST(RunnerTest, CreateOutputDirectoriesTestWithOutputPathsField)
{
    TestRunner runner;
    const std::string cwd = SystemUtils::get_current_working_directory();

    const std::vector<std::string> output_paths{
        "build_t/intermediate", "tmp_t/build",         "empty",    "",
        "intermediate_t/tmp.o", "artifacts_t/build.o", "empty.txt"};

    const std::vector<std::string> expected_directories{
        "build_t", "tmp_t", "intermediate_t", "artifacts_t"};

    for (const auto &dir : expected_directories) {
        std::string full_path = cwd + "/" + dir;
        // directories should not exist
        EXPECT_FALSE(FileUtils::isDirectory(full_path.c_str()));
    }

    // Use v2.1's `output_path` field instead of `output_{files, directories}`:
    Command command;
    for (const auto &path : output_paths) {
        *command.add_output_paths() = path;
    }

    runner.testCreateOutputDirectories(command, cwd);

    for (const auto &dir : expected_directories) {
        std::string full_path = cwd + "/" + dir;
        // directories should now exist
        EXPECT_TRUE(FileUtils::isDirectory(full_path.c_str()));
        // clean up directory now
        FileUtils::deleteDirectory(full_path.c_str());
    }
}

TEST(RunnerTest,
     CreateOutputDirectoriesDeprecatedFieldsIgnoredIfOutputPathIsSet)
{
    TestRunner runner;
    const std::string cwd = SystemUtils::get_current_working_directory();

    const std::vector<std::string> output_paths{"build/a.out", "output"};

    const std::string expected_directory = cwd + "/build";
    EXPECT_FALSE(FileUtils::isDirectory(expected_directory.c_str()));

    // According to the REAPI: "If `output_paths` is used, `output_files` and
    // `output_directories` will be ignored".
    Command command;
    for (const auto &path : output_paths) {
        *command.add_output_paths() = path;
    }
    // Then setting these fields should have no effect:
    *command.add_output_directories() = "ignored-directory";
    *command.add_output_files() = "ignored-file-parent-directory/ignored-file";

    runner.testCreateOutputDirectories(command, cwd);

    ASSERT_FALSE(FileUtils::isDirectory(
        std::string(cwd + "ignored-file-parent-directory").c_str()));
    ASSERT_FALSE(FileUtils::isDirectory(
        std::string(cwd + "ignored-directory").c_str()));

    ASSERT_TRUE(FileUtils::isDirectory(expected_directory.c_str()));
    FileUtils::deleteDirectory(expected_directory.c_str());
}

TEST(RunnerTest, ChmodDirectory)
{
    TemporaryDirectory dir;
    const std::string subdirectory_path = std::string(dir.name()) + "/subdir";

    mode_t perm = 0555;
    // create subdirectory with restrictive permissions.
    FileUtils::createDirectory(subdirectory_path.c_str(), perm);

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

TEST(RunnerTest, CustomStandardOutputDestinations)
{
    TestRunner runner;

    // Redirecting standard outputs to files:
    TemporaryFile stdout_file;
    runner.setStdOutFile(stdout_file.strname());
    TemporaryFile stderr_file;
    runner.setStdErrFile(stderr_file.strname());

    ActionResult result;

    const auto path_to_echo = SystemUtils::getPathToCommand("echo");
    ASSERT_FALSE(path_to_echo.empty());

    runner.executeAndStore({path_to_echo, "hello", "world"},
                           TestRunner::dummyUploadFunction, &result);

    const auto expected_stdout = "hello world\n";
    EXPECT_EQ(result.stdout_digest(), CASHash::hash(expected_stdout));
    EXPECT_TRUE(result.stdout_raw().empty()); // `Runner` does not inline.
    ASSERT_EQ(FileUtils::getFileContents(stdout_file.name()), expected_stdout);

    EXPECT_EQ(result.stderr_digest(), CASHash::hash(""));
    ASSERT_EQ(FileUtils::getFileContents(stderr_file.name()), "");

    EXPECT_EQ(result.exit_code(), 0);
}
