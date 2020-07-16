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

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_stageddirectory.h>
#include <buildboxcommon_temporarydirectory.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

// We want to test the algorithm in `StagedDirectory::captureAllOutputs()`
// which is common to all `StagedDirectory` implementations. It is responsible
// for handling the paths in a `Command` and verifying that they are valid.
class MockStagedDirectory : public StagedDirectory, public testing::Test {
  protected:
    MockStagedDirectory() : StagedDirectory()
    {
        d_dummy_capture_file_callback = [&](const char *) {
            return OutputFile();
        };
        d_dummy_capture_directory_callback = [&](const char *) {
            return OutputDirectory();
        };
    }

    ~MockStagedDirectory() override {}

    OutputFile captureFile(const char *, const Command &) const override
    {
        return OutputFile();
    }

    OutputDirectory captureDirectory(const char *,
                                     const Command &) const override
    {
        return OutputDirectory();
    }

    StagedDirectory::CaptureFileCallback d_dummy_capture_file_callback;

    StagedDirectory::CaptureDirectoryCallback
        d_dummy_capture_directory_callback;

    void assert_capturing_throws(const Command &command) const
    {
        ActionResult action_result;
        ASSERT_THROW(
            this->captureAllOutputs(command, &action_result,
                                    d_dummy_capture_file_callback,
                                    d_dummy_capture_directory_callback),
            std::invalid_argument);
    }
};

TEST_F(MockStagedDirectory, DirectoryPathsWithLeadingOrTrailingSlashesThrow)
{
    // According to the REAPI: "The path MUST NOT include a trailing slash, nor
    // a leading slash, being a relative path."
    const auto illegal_paths = {"subdir/", "/subdir", "/subdir/"};

    for (const auto &path : illegal_paths) {
        Command command;
        *command.add_output_directories() = path;

        ActionResult action_result;
        ASSERT_THROW(
            this->captureAllOutputs(command, &action_result,
                                    d_dummy_capture_file_callback,
                                    d_dummy_capture_directory_callback),
            std::invalid_argument);
    }
}

TEST_F(MockStagedDirectory, EmptyDirectoryPathIsAllowed)
{
    Command command;
    *command.add_output_directories() = "";

    bool captured_empty_path = false;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_empty_path = (strlen(relative_path) == 0);
            return OutputDirectory();
        };

    ActionResult action_result;
    ASSERT_NO_THROW(this->captureAllOutputs(command, &action_result,
                                            d_dummy_capture_file_callback,
                                            capture_directory_function));
}

TEST_F(MockStagedDirectory, EmptyOutputPathIsAllowed)
{
    Command command;
    *command.add_output_paths() = "";

    bool captured_empty_path = false;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_empty_path = (strlen(relative_path) == 0);
            return OutputDirectory();
        };

    ActionResult action_result;
    ASSERT_NO_THROW(this->captureAllOutputs(command, &action_result,
                                            d_dummy_capture_file_callback,
                                            capture_directory_function));
}

TEST_F(MockStagedDirectory, WorkingDirectoryOutsideInputRootThrows)
{
    Command command;
    command.set_working_directory("../out-of-input-root");
    *command.add_output_files() = "a.out";

    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory,
       WorkingDirectoryOutsideInputRootAndOutputPathThrows)
{
    Command command;
    command.set_working_directory("../out-of-input-root");
    *command.add_output_paths() = "a.out";

    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, FilePathWithLeadingSlashThrows)
{
    Command command;
    *command.add_output_files() = "/a.out";

    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, OutputPathWithLeadingSlashThrows)
{
    Command command;
    *command.add_output_paths() = "/a.out";

    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, PathsOutsideInputRootThrow)
{
    Command command;
    *command.add_output_files() = "../a.out";

    ActionResult action_result;
    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, OutputPathsOutsideInputRootThrow)
{
    Command command;
    *command.add_output_paths() = "../a.out";

    ActionResult action_result;
    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, PathsOutsideInputRootWithWorkingDirThrows)
{
    Command command;
    // a.out
    // input_root/
    //          | src/  <-- working dir (1 level down)

    command.set_working_directory("src");
    *command.add_output_files() = "../../a.out";
    // ^ path above input root: this is not allowed.

    assert_capturing_throws(command);
}

TEST_F(MockStagedDirectory, OutputPathsOutsideInputRootWithWorkingDirThrows)
{
    Command command;
    // a.out
    // input_root/
    //          | src/  <-- working dir (1 level down)

    command.set_working_directory("src");
    *command.add_output_paths() = "../../a.out";
    // ^ path above input root: this is not allowed.

    assert_capturing_throws(command);
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

TEST_F(MockStagedDirectory, CommandWorkingDirectoryWithOutputPathsField)
{
    std::vector<std::string> captured_file_paths;
    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *relative_path) {
            captured_file_paths.push_back(relative_path);
            return OutputFile();
        };

    std::vector<std::string> captured_directory_paths;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_directory_paths.push_back(relative_path);
            return OutputDirectory();
        };

    TemporaryDirectory working_directory;

    Command command;
    command.set_working_directory("working-directory");
    // Using v2.1's `output_path` field:
    *command.add_output_paths() = "subdirectory";
    *command.add_output_paths() = "file1.txt";

    // According to the spec, when the new field is set, the deprecated ones
    // are ignored:
    *command.add_output_directories() = "ignored-subdirectory";
    *command.add_output_files() = "ignored-file.txt";

    FileUtils::createDirectory("working-directory/subdirectory");
    FileUtils::createDirectory("working-directory/ignored-subdirectory");

    FileUtils::writeFileAtomically("working-directory/file1.txt", "");
    FileUtils::writeFileAtomically("working-directory/ignored-file.txt", "");

    // According to the spec, once that is set, the old fields are ignored:
    //    *command.add_output_directories() = "subdirectory";
    //    *command.add_output_files() = "file1.txt";

    ActionResult action_result;
    this->captureAllOutputs(command, &action_result, capture_file_function,
                            capture_directory_function);

    EXPECT_EQ(captured_file_paths.size(), 1);
    EXPECT_EQ(captured_file_paths.at(0), "working-directory/file1.txt");

    EXPECT_EQ(captured_file_paths.size(), 1);
    EXPECT_EQ(captured_directory_paths.at(0),
              "working-directory/subdirectory");

    FileUtils::deleteDirectory("working-directory");
}

TEST_F(MockStagedDirectory,
       CommandWorkingDirectoryWithOutputPathsFieldContainingSymlink)
{
    std::vector<std::string> captured_file_paths;
    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *relative_path) {
            captured_file_paths.push_back(relative_path);
            return OutputFile();
        };

    std::vector<std::string> captured_directory_paths;
    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_directory_paths.push_back(relative_path);
            return OutputDirectory();
        };

    TemporaryDirectory working_directory;

    Command command;
    command.set_working_directory("working-directory");
    // Using v2.1's `output_path` field:
    *command.add_output_paths() = "symlink";

    FileUtils::createDirectory("working-directory");
    FileUtils::writeFileAtomically("working-directory/file.txt", "");
    ASSERT_EQ(
        symlink("working-directory/file.txt", "working-directory/symlink"), 0);

    ActionResult action_result;
    ASSERT_THROW(this->captureAllOutputs(command, &action_result,
                                         capture_file_function,
                                         capture_directory_function),
                 std::invalid_argument);

    FileUtils::deleteDirectory("working-directory");
}

class OpenDirectoryInInputRootFixture : public ::testing::Test {
  protected:
    OpenDirectoryInInputRootFixture() : root_directory_fd(-1)
    {
        // Testing with the following directory structure:
        //
        // * root_directory/      symlink
        //      | subdir1/  <--------------------|
        //           | subdir2/                  |
        //               | file.txt              |
        //               | symlink/ -------------|

        const std::string root_directory_path = root_directory.name();

        const auto subdir1_path = root_directory_path + "/subdir1/";
        FileUtils::createDirectory(subdir1_path.c_str());

        const auto subdir2_path = root_directory_path + "/subdir1/subdir2/";
        FileUtils::createDirectory(subdir2_path.c_str());

        const auto subdir3_path =
            root_directory_path + "/subdir1/subdir2/symlink";

        if (symlink(subdir1_path.c_str(), subdir3_path.c_str()) == -1) {
            throw std::system_error(
                errno, std::system_category(),
                "Error creating symlink in the test directory structure.");
        }

        FileUtils::writeFileAtomically(
            std::string(root_directory_path + "/subdir1/subdir2/file.txt")
                .c_str(),
            "Some data...");

        root_directory_fd =
            open(root_directory.name(), O_DIRECTORY | O_RDONLY);
    }

    TemporaryDirectory root_directory;
    int root_directory_fd;

    ~OpenDirectoryInInputRootFixture() { close(root_directory_fd); }
};

void assertFileInDirectory(const int dir_fd, const std::string &filename)
{
    ASSERT_NE(dir_fd, -1);

    int file_fd = openat(dir_fd, filename.c_str(), O_RDONLY);
    ASSERT_NE(file_fd, -1);

    close(file_fd);
}

TEST_F(OpenDirectoryInInputRootFixture, ValidPath)
{
    int directory_fd = -1;
    ASSERT_NO_THROW(directory_fd =
                        StagedDirectoryUtils::openDirectoryInInputRoot(
                            root_directory_fd, "subdir1/subdir2"));

    assertFileInDirectory(directory_fd, "file.txt");

    close(directory_fd);
}

TEST_F(OpenDirectoryInInputRootFixture, OpenInputRoot)
{
    int directory_fd = -1;
    ASSERT_NO_THROW(directory_fd =
                        StagedDirectoryUtils::openDirectoryInInputRoot(
                            root_directory_fd, "."));

    ASSERT_NE(directory_fd, -1);

    close(directory_fd);
}

TEST_F(OpenDirectoryInInputRootFixture, ValidPaths)
{
    int subdir1_fd = -1;
    ASSERT_NO_THROW(subdir1_fd =
                        StagedDirectoryUtils::openDirectoryInInputRoot(
                            root_directory_fd, "subdir1/"));
    ASSERT_NE(subdir1_fd, -1);

    int subdir2_fd = -1;
    ASSERT_NO_THROW(subdir2_fd =
                        StagedDirectoryUtils::openDirectoryInInputRoot(
                            subdir1_fd, "subdir2/"));
    ASSERT_NE(subdir2_fd, -1);

    assertFileInDirectory(subdir2_fd, "file.txt");

    close(subdir1_fd);
    close(subdir2_fd);
}

TEST_F(OpenDirectoryInInputRootFixture, RootFDArgumentIsNotClosed)
{
    const int directory_fd = StagedDirectoryUtils::openDirectoryInInputRoot(
        root_directory_fd, "subdir1/subdir2");

    ASSERT_NE(fcntl(root_directory_fd, F_GETFD), -1);

    close(directory_fd);
}

TEST_F(OpenDirectoryInInputRootFixture, OpenFile)
{
    ASSERT_THROW(StagedDirectoryUtils::openDirectoryInInputRoot(
                     root_directory_fd, "subdir1/subdir2/file.txt"),
                 std::runtime_error);
}

TEST_F(OpenDirectoryInInputRootFixture, SymlinkInsideRoot)
{
    ASSERT_THROW(StagedDirectoryUtils::openDirectoryInInputRoot(
                     root_directory_fd, "subdir1/subdir2/symlink"),
                 std::system_error);
}

TEST_F(OpenDirectoryInInputRootFixture, SymlinkEscapingRoot)
{
    // Trying to open "subdir1/subdir2/symlink" with `subdir2/` as the root.
    // `symlink` points to `subdir1/` is one level above and
    // therefore not allowed.
    int subdir2_fd =
        openat(root_directory_fd, "subdir1/subdir2/", O_DIRECTORY | O_RDONLY);

    ASSERT_THROW(StagedDirectoryUtils::openDirectoryInInputRoot(
                     subdir2_fd, "subdir1/subdir2/symlink"),
                 std::system_error);
}

TEST_F(OpenDirectoryInInputRootFixture, FileInInputRoot)
{
    ASSERT_TRUE(StagedDirectoryUtils::fileInInputRoot(
        root_directory_fd, "subdir1/subdir2/file.txt"));

    ASSERT_FALSE(StagedDirectoryUtils::fileInInputRoot(
        root_directory_fd, "subdir1/subdir2/non-existing-file.txt"));

    ASSERT_FALSE(StagedDirectoryUtils::fileInInputRoot(root_directory_fd,
                                                       "subdir1/subdir2/"));

    ASSERT_FALSE(StagedDirectoryUtils::fileInInputRoot(
        root_directory_fd, "subdir1/subdir2/symlink"));
}

TEST_F(OpenDirectoryInInputRootFixture, DirectoryInInputRoot)
{

    ASSERT_TRUE(StagedDirectoryUtils::directoryInInputRoot(root_directory_fd,
                                                           "subdir1/subdir2"));

    ASSERT_TRUE(StagedDirectoryUtils::directoryInInputRoot(root_directory_fd,
                                                           "subdir1/"));

    ASSERT_TRUE(
        StagedDirectoryUtils::directoryInInputRoot(root_directory_fd, "."));

    ASSERT_FALSE(StagedDirectoryUtils::directoryInInputRoot(
        root_directory_fd, "subdir1/subdir2/file.txt"));

    ASSERT_FALSE(StagedDirectoryUtils::directoryInInputRoot(
        root_directory_fd, "subdir1/subdir2/non-existing-file.txt"));

    ASSERT_FALSE(StagedDirectoryUtils::directoryInInputRoot(
        root_directory_fd, "subdir1/subdir2/symlink"));
}
