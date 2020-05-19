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

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

void StagedDirectory::captureAllOutputs(const Command &command,
                                        ActionResult *result) const
{
    CaptureFileCallback capture_file_function = [&](const char *path) {
        return this->captureFile(path, command);
    };

    CaptureDirectoryCallback capture_directory_function =
        [&](const char *path) {
            return this->captureDirectory(path, command);
        };

    return captureAllOutputs(command, result, capture_file_function,
                             capture_directory_function);
}

void StagedDirectory::captureAllOutputs(
    const Command &command, ActionResult *result,
    StagedDirectory::CaptureFileCallback capture_file_function,
    StagedDirectory::CaptureDirectoryCallback capture_directory_function) const
{

    std::string base_path;
    if (!command.working_directory().empty()) {
        // According to the REAPI, `Command.working_directory()` can be empty.
        // In that case, we want to avoid adding leading slashes to paths:
        // that would make them absolute. To simplify handling this later, we
        // add the trailing slash here.
        base_path =
            FileUtils::normalizePath(command.working_directory().c_str()) +
            "/";

        if (base_path.front() == '/') {
            const auto error_message = "`working_directory` path in `Command` "
                                       "must be relative. It is \"" +
                                       base_path + "\"";
            BUILDBOX_LOG_ERROR(error_message);
            throw std::invalid_argument(error_message);
        }

        if (base_path.substr(0, 3) == "../") {
            const auto error_message =
                "The `working_directory` path in `Command` is "
                "outside of input root \"" +
                base_path + "\"";
            BUILDBOX_LOG_ERROR(error_message);
            throw std::invalid_argument(error_message);
        }
    }

    // Also:
    //  "The paths are relative to the working directory of the action
    //   execution. [...] The path MUST NOT include a trailing slash,
    //   nor a leading slash, being a relative path."
    const auto assert_no_invalid_slashes = [](const std::string &path) {
        if (path.front() == '/' || path.back() == '/') {
            const auto error_message = "Output path in `Command` has "
                                       "leading or trailing slashes: \"" +
                                       path + "\"";
            BUILDBOX_LOG_ERROR(error_message);
            throw std::invalid_argument(error_message);
        }
    };

    const auto path_in_input_root = [&base_path](const std::string &path) {
        return FileUtils::normalizePath((base_path + path).c_str());
    };

    const auto assert_path_inside_input_root = [](const std::string
                                                      &path_from_root) {
        // PRE: `path_from_root` is normalized.
        if (path_from_root == ".." || path_from_root.substr(0, 3) == "../") {
            const auto error_message =
                "Output path in `Command` is outside of the input root: \"" +
                path_from_root + "\"";
            BUILDBOX_LOG_ERROR(error_message);
            throw std::invalid_argument(error_message);
        }
    };

    for (const auto &outputFilename : command.output_files()) {
        assert_no_invalid_slashes(outputFilename);
        const std::string path = path_in_input_root(outputFilename);
        assert_path_inside_input_root(path);

        OutputFile outputFile = capture_file_function(path.c_str());
        if (!outputFile.path().empty()) {
            outputFile.set_path(outputFilename);
            OutputFile *file_entry = result->add_output_files();
            file_entry->CopyFrom(outputFile);
        }
    }

    for (const auto &outputDirName : command.output_directories()) {
        assert_no_invalid_slashes(outputDirName);
        const std::string path = path_in_input_root(outputDirName);
        assert_path_inside_input_root(path);

        OutputDirectory outputDirectory =
            capture_directory_function(path.c_str());
        if (!outputDirectory.path().empty()) {
            outputDirectory.set_path(outputDirName);
            OutputDirectory *directory_entry =
                result->add_output_directories();
            directory_entry->CopyFrom(outputDirectory);
        }
    }
}

int StagedDirectoryUtils::openFileInInputRoot(const int root_dir_fd,
                                              const std::string &relative_path)
{
    // Splitting the path into a path and a filename and opening the directory
    // where the file is at:
    const auto path = std::string(relative_path);
    const auto lastSlash = path.find_last_of('/');

    std::string filename;
    int directory_fd;
    if (lastSlash == std::string::npos) {
        // The path is a file in the root of the stage directory, we already
        // have that directory open:
        directory_fd = root_dir_fd;
        filename = path;
    }
    else {
        // If not, we open the directory making sure that there are no
        // symlinks in it:
        const auto base_path = path.substr(0, lastSlash);
        directory_fd = openDirectoryInInputRoot(root_dir_fd, base_path);

        filename = path.substr(lastSlash + 1);
    }

    // Now that we have the directory that contains the file open, and we are
    // certain that it is inside the input root, we can open the file (also
    // making sure that it is not a symlink):
    const int file_fd =
        openat(directory_fd, filename.c_str(), O_RDONLY | O_NOFOLLOW);

    if (directory_fd != root_dir_fd) {
        // Do not close the FD given as argument.
        close(directory_fd);
    }

    if (file_fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening \"" << relative_path << "\"");
    }

    return file_fd;
}

int StagedDirectoryUtils::openDirectoryInInputRoot(const int root_dir_fd,
                                                   const std::string &path)
{
    // Removing trailing slashes to simplify detecting the end for all cases.
    const auto path_end = path.cend() - (path.back() == '/');

    auto findNextSeparator =
        [&path_end](const std::string::const_iterator &start) {
            // Return where the next directory component in `path` ends.
            const auto separator = '/';
            return std::find(start + 1, path_end, separator);
        };

    auto subdir_start = path.cbegin();
    auto subdir_end = findNextSeparator(subdir_start);

    int current_dir_fd = root_dir_fd;
    while (true) {
        const auto subdir_path = std::string(subdir_start, subdir_end);

        const int subdir_fd = openat(current_dir_fd, subdir_path.c_str(),
                                     O_DIRECTORY | O_NOFOLLOW);

        if (subdir_fd == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error opening subdirectory " << subdir_path << " in path \""
                                              << path << "\"");
        }

        if (subdir_end == path_end) { // We opened the last directory.
            return subdir_fd;
        }

        // Going down to the next level:
        if (current_dir_fd != root_dir_fd) {
            close(current_dir_fd); // Do not close the fd given as an argument.
        }
        current_dir_fd = subdir_fd;

        subdir_start = subdir_end + 1;
        subdir_end = findNextSeparator(subdir_start);
    }
}

bool StagedDirectoryUtils::fileInInputRoot(const int root_dir_fd,
                                           const std::string &path)
{
    try {
        const int fd = openFileInInputRoot(root_dir_fd, path);
        close(fd);
        return true;
    }
    catch (const std::system_error &) {
        return false;
    }
}

bool StagedDirectoryUtils::directoryInInputRoot(const int root_dir_fd,
                                                const std::string &path)
{
    if (path.empty()) {
        return true;
    }

    try {
        const int fd = openDirectoryInInputRoot(root_dir_fd, path);
        close(fd);
        return true;
    }
    catch (const std::system_error &) {
        return false;
    }
}
