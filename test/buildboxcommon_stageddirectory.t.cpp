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

#include <buildboxcommon_stageddirectory.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

// We want to test the algorithm in `StagedDirectory::captureAllOutputs()`
// which is common to all `StagedDirectory` implementations. It is responsible
// for handling the paths in a `Command` and verifying that they are valid.
class MockStagedDirectory : public StagedDirectory, public testing::Test {
  protected:
    MockStagedDirectory() : StagedDirectory() {}
    ~MockStagedDirectory() override {}

    OutputFile captureFile(const char *) const override
    {
        return OutputFile();
    }

    OutputDirectory captureDirectory(const char *) const override
    {
        return OutputDirectory();
    }
};

TEST_F(MockStagedDirectory, DirectoryPathsWithLeadingOrTrailingSlashesThrow)
{
    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *) { return OutputFile(); };

    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *) { return OutputDirectory(); };

    // According to the REAPI: "The path MUST NOT include a trailing slash, nor
    // a leading slash, being a relative path."
    const auto illegal_paths = {"subdir/", "/subdir", "/subdir/"};

    for (const auto &path : illegal_paths) {
        Command command;
        *command.add_output_directories() = path;

        ActionResult action_result;
        ASSERT_THROW(this->captureAllOutputs(command, &action_result,
                                             capture_file_function,
                                             capture_directory_function),
                     std::invalid_argument);
    }
}

TEST_F(MockStagedDirectory, EmptyDirectoryPathIsAllowed)
{
    Command command;
    *command.add_output_directories() = "";

    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *) { return OutputFile(); };

    bool captured_empty_path = false;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_empty_path = (strlen(relative_path) == 0);
            return OutputDirectory();
        };

    ActionResult action_result;
    ASSERT_NO_THROW(this->captureAllOutputs(command, &action_result,
                                            capture_file_function,
                                            capture_directory_function));
}

TEST_F(MockStagedDirectory, FilePathWithLeadingSlashThrows)
{
    Command command;
    *command.add_output_files() = "/a.out";

    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *) { return OutputFile(); };

    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *) { return OutputDirectory(); };

    ActionResult action_result;
    ASSERT_THROW(this->captureAllOutputs(command, &action_result,
                                         capture_file_function,
                                         capture_directory_function),
                 std::invalid_argument);
}

TEST_F(MockStagedDirectory, CommandWorkingDirectory)
{
    std::string captured_file_path;
    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *relative_path) {
            captured_file_path = std::string(relative_path);
            return OutputFile();
        };

    std::string captured_directory_path;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_directory_path = std::string(relative_path);
            return OutputDirectory();
        };

    Command command;
    command.set_working_directory("working-directory");
    *command.add_output_directories() = "subdirectory";
    *command.add_output_files() = "file1.txt";

    ActionResult action_result;
    this->captureAllOutputs(command, &action_result, capture_file_function,
                            capture_directory_function);

    ASSERT_EQ(captured_file_path, "working-directory/file1.txt");
    ASSERT_EQ(captured_directory_path, "working-directory/subdirectory");
}
