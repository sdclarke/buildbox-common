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

FallbackStagedDirectory::FallbackStagedDirectory(
    const Digest &digest, const std::string &path,
    std::shared_ptr<Client> cas_client)
    : d_casClient(cas_client), d_stage_directory(path.c_str(), "buildboxrun")
{
    this->d_path = d_stage_directory.name();
    BUILDBOX_LOG_DEBUG("Downloading to " << this->d_path);
    this->downloadDirectory(digest, this->d_path.c_str());
}

FallbackStagedDirectory::~FallbackStagedDirectory() {}

OutputFile
FallbackStagedDirectory::captureFileWithFD(const int dirFD,
                                           const char *relative_path) const
{
    BUILDBOX_LOG_DEBUG("Uploading " << relative_path);

    const int fd = openat(dirFD, relative_path, O_RDONLY);
    if (fd == -1) {
        if (errno == EACCES || errno == ENOENT) {
            return OutputFile();
        }
        throw std::system_error(errno, std::system_category());
    }
    if (FileUtils::is_directory(fd)) {
        close(fd);
        return OutputFile();
    }

    const Digest digest = CASHash::hash(fd);
    OutputFile output_file;

    try {
        d_casClient->upload(fd, digest);
        output_file.set_is_executable(FileUtils::is_executable(fd));
        close(fd);
    }
    catch (...) {
        close(fd);
        throw;
    }
    output_file.set_path(relative_path);
    output_file.mutable_digest()->CopyFrom(digest);
    return output_file;
}

OutputFile
FallbackStagedDirectory::captureFile(const char *relative_path) const
{
    return StagedDirectory::captureFile(relative_path, this->d_path.c_str(),
                                        this->d_casClient);
}

Directory
FallbackStagedDirectory::uploadDirectoryRecursively(Tree *tree,
                                                    const int dirFD) const
{
    std::map<std::string, FileNode> files;
    std::map<std::string, DirectoryNode> directories;
    // when the DIR stream closes, so will the underlying file descriptor
    std::unique_ptr<DIR, int (*)(DIR *)> dirStream(fdopendir(dirFD),
                                                   &closedir);
    for (auto entry = readdir(dirStream.get()); entry != nullptr;
         entry = readdir(dirStream.get())) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            // Skip "." and ".."
            continue;
        }
        std::cout << entry->d_name << std::endl;
        // Check if the path is a file.
        OutputFile outputFile = this->captureFileWithFD(dirFD, entry->d_name);
        if (!outputFile.path().empty()) {
            FileNode fileNode;
            fileNode.set_name(entry->d_name);
            fileNode.mutable_digest()->CopyFrom(outputFile.digest());
            fileNode.set_is_executable(outputFile.is_executable());
            files[entry->d_name] = fileNode;
            continue;
        }

        // Try uploading the path as a directory.
        const int subDirFD =
            openat(dirFD, entry->d_name, O_DIRECTORY | O_RDONLY);
        if (subDirFD == -1) {
            throw std::system_error(errno, std::system_category());
        }
        BUILDBOX_LOG_DEBUG("Uploading directory " << entry->d_name);
        Directory directory = uploadDirectoryRecursively(tree, subDirFD);

        DirectoryNode directoryNode;
        directoryNode.set_name(entry->d_name);
        directoryNode.mutable_digest()->CopyFrom(
            this->d_casClient->uploadMessage<Directory>(directory));
        *(tree->add_children()) = directory;
        directories[entry->d_name] = directoryNode;
    }

    Directory result;
    // NOTE: we're guaranteed to iterate over these in alphabetical order
    // since we're using a std::map
    for (const auto &filePair : files) {
        *(result.add_files()) = filePair.second;
    }
    for (const auto &directoryPair : directories) {
        *(result.add_directories()) = directoryPair.second;
    }
    return result;
}

OutputDirectory
FallbackStagedDirectory::captureDirectory(const char *relative_path) const
{
    Tree tree;
    std::string path = relative_path;
    const std::string dir_path =
        FileUtils::make_path_absolute(path, this->d_path);
    // using AT_FDCWD as placeholder. Since dirPath is absolute, openAT will
    // ignore AT_FDCWD
    const int dirFD =
        openat(AT_FDCWD, dir_path.c_str(), O_DIRECTORY | O_RDONLY);
    if (dirFD == -1) {
        throw std::system_error(errno, std::system_category());
    }

    BUILDBOX_LOG_DEBUG("Uploading directory " << relative_path);
    // dirFD will be closed by uploadDirectoryRecursively
    *(tree.mutable_root()) = this->uploadDirectoryRecursively(&tree, dirFD);

    const Digest tree_digest = this->d_casClient->uploadMessage<Tree>(tree);

    OutputDirectory output_directory;
    output_directory.set_path(relative_path);
    output_directory.mutable_tree_digest()->CopyFrom(tree_digest);
    return output_directory;
}

void FallbackStagedDirectory::downloadFile(const Digest &digest,
                                           bool executable,
                                           const char *path) const
{
    BUILDBOX_LOG_DEBUG("Downloading file with digest " << toString(digest)
                                                       << " to " << path);
    const int fd =
        open(path, O_CREAT | O_WRONLY | O_TRUNC, executable ? 0755 : 0644);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    try {
        this->d_casClient->download(fd, digest);
    }
    catch (...) {
        close(fd);
        throw;
    }
    close(fd);
}

void FallbackStagedDirectory::downloadDirectory(const Digest &digest,
                                                const char *path) const
{
    BUILDBOX_LOG_DEBUG("Downloading directory with digest " << toString(digest)
                                                            << " to " << path);

    try {
        this->d_casClient->downloadDirectory(digest, path);
    }
    catch (const std::exception &e) {
        BUILDBOX_LOG_DEBUG("Could not download directory with digest "
                           << toString(digest) << " to " << path << ": "
                           << e.what());
        throw;
    }
}

} // namespace buildboxcommon
