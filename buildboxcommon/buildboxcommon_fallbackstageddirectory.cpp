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
        throw std::system_error(errno, std::system_category());
    }

    BUILDBOX_LOG_DEBUG("Downloading to " << this->d_path);
    this->d_casClient->downloadDirectory(digest, this->d_path.c_str());
}

FallbackStagedDirectory::~FallbackStagedDirectory()
{
    BUILDBOX_LOG_DEBUG("Unstaging " << this->d_path);
    close(this->d_stage_directory_fd);
}

OutputFile
FallbackStagedDirectory::captureFile(const char *relative_path) const
{

    const auto upload_file_function = [this](const int fd,
                                             const Digest &digest) {
        this->d_casClient->upload(fd, digest);
    };

    return captureFile(relative_path, upload_file_function);
}

OutputDirectory
FallbackStagedDirectory::captureDirectory(const char *relative_path) const
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
    const std::string absolute_path =
        FileUtils::make_path_absolute(relative_path, this->d_path);

    if (!buildboxcommon::FileUtils::is_directory(absolute_path.c_str())) {
        return OutputDirectory();
    }

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
    output_file.set_is_executable(FileUtils::is_executable(fd));

    close(fd);

    return output_file;
}

int FallbackStagedDirectory::openFile(const char *relative_path) const
{
    const int fd = openat(this->d_stage_directory_fd, relative_path, O_RDONLY);
    if (fd == -1) {
        const int openat_error = errno;
        BUILDBOX_LOG_ERROR("Error opening \""
                           << relative_path << "\" inside of \""
                           << this->d_stage_directory.name()
                           << "\": " << strerror(openat_error));

        throw std::system_error(openat_error, std::system_category());
    }

    return fd;
}

} // namespace buildboxcommon
