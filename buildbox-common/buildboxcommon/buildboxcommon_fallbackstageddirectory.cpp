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

#include <buildboxcommon_fallbackstageddirectory.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>

#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace buildboxcommon {

FallbackStagedDirectory::FallbackStagedDirectory() {}

FallbackStagedDirectory::FallbackStagedDirectory(
    const Digest &digest, const std::string &path,
    std::shared_ptr<Client> cas_client)
    : d_casClient(cas_client), d_stage_directory(path.c_str(), "buildboxrun")
{
    this->d_path = d_stage_directory.name();

    // Using `AT_FDCWD` as placeholder. Since `d_path` is absolute, `openat()`
    // will ignore it.
    this->d_stage_directory_fd =
        openat(AT_FDCWD, this->d_path.c_str(), O_DIRECTORY | O_RDONLY);
    // (It will be closed by the destructor.)

    if (this->d_stage_directory_fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening directory path \"" << path << "\"");
    }

    BUILDBOX_LOG_DEBUG("Downloading to " << this->d_path);
    this->d_casClient->downloadDirectory(digest, this->d_path.c_str());
}

FallbackStagedDirectory::~FallbackStagedDirectory()
{
    BUILDBOX_LOG_DEBUG("Unstaging " << this->d_path);
    close(this->d_stage_directory_fd);
}

OutputFile FallbackStagedDirectory::captureFile(const char *relative_path,
                                                const Command &) const
{

    const auto upload_file_function = [this](const int fd,
                                             const Digest &digest) {
        this->d_casClient->upload(fd, digest);
    };

    return captureFile(relative_path, upload_file_function);
}

OutputDirectory
FallbackStagedDirectory::captureDirectory(const char *relative_path,
                                          const Command &) const
{

    const auto upload_directory_function = [this](const std::string &path) {
        return this->uploadDirectory(path);
    };

    return this->captureDirectory(relative_path, upload_directory_function);
}

OutputDirectory FallbackStagedDirectory::captureDirectory(
    const char *relative_path,
    const std::function<Digest(const std::string &path)>
        &upload_directory_function) const
{

    try {
        // Attempting to open the directory without following symlinks.
        // If this succeeds, it means that the path is valid: pointing to a
        // directory inside the input root.
        const int directory_fd =
            openDirAt(this->d_stage_directory_fd, relative_path);
        close(directory_fd);
    }
    catch (const std::system_error &) {
        return OutputDirectory();
    }

    const std::string absolute_path =
        FileUtils::makePathAbsolute(relative_path, this->d_path);
    const Digest tree_digest = upload_directory_function(absolute_path);

    OutputDirectory output_directory;
    output_directory.set_path(relative_path);
    output_directory.mutable_tree_digest()->CopyFrom(tree_digest);
    return output_directory;
}

Digest FallbackStagedDirectory::uploadDirectory(const std::string &path) const
{
    BUILDBOX_LOG_DEBUG("Uploading directory " << path);

    Digest root_directory_digest;
    Tree tree;
    this->d_casClient->uploadDirectory(path, &root_directory_digest, &tree);

    const Digest tree_digest = this->d_casClient->uploadMessage(tree);
    return tree_digest;
}

OutputFile FallbackStagedDirectory::captureFile(
    const char *relative_path,
    const std::function<void(const int fd, const Digest &digest)>
        &upload_file_function) const
{
    int fd;
    try {
        fd = this->openFile(relative_path);
    }
    catch (const std::system_error &e) {
        if (e.code().value() == ENOENT) {
            return OutputFile();
        }

        throw;
    }

    const Digest digest = CASHash::hash(fd);

    try {
        upload_file_function(fd, digest);
    }
    catch (...) {
        close(fd);
        throw;
    }

    OutputFile output_file;
    output_file.set_path(relative_path);
    output_file.mutable_digest()->CopyFrom(digest);
    output_file.set_is_executable(FileUtils::isExecutable(fd));

    close(fd);

    return output_file;
}

int FallbackStagedDirectory::openFile(const char *relative_path) const
{
    // `relative_path` is guaranteed to be inside the input root by the checks
    // performed by `StagedDirectory`, but we still want to make sure that none
    // of its components is a symlink that points to outside the input root.
    // For simplicity, we won't follow any symlinks.

    // Splitting the path into a path and a filename and opening the directory
    // where the file is at:
    const auto path = std::string(relative_path);
    const auto lastSlash = path.find_last_of('/');

    std::string filename;
    int directory_fd;
    if (lastSlash == std::string::npos) {
        // The path is a file in the root of the stage directory, we already
        // have that directory open:
        directory_fd = this->d_stage_directory_fd;
        filename = path;
    }
    else {
        // If not, we we open the directory making sure that there are no
        // symlinks in it:
        const auto base_path = path.substr(0, lastSlash);
        directory_fd = openDirAt(this->d_stage_directory_fd, base_path);

        filename = path.substr(lastSlash + 1);
    }

    // Now that we have the directory that contains the file open, and we are
    // certain that it is inside the input root, we can open the file (also
    // making sure that it is not a symlink):
    const int file_fd =
        openat(directory_fd, filename.c_str(), O_RDONLY | O_NOFOLLOW);

    if (directory_fd != this->d_stage_directory_fd) {
        close(directory_fd);
    }

    if (file_fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening \"" << relative_path << "\" inside of \""
                               << this->d_stage_directory.name() << "\"");
    }

    return file_fd;
}

int FallbackStagedDirectory::openDirAt(const int root_dir_fd,
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

} // namespace buildboxcommon
