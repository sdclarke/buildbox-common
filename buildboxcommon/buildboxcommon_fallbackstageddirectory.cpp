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
    const Digest &digest, std::shared_ptr<Client> casClient)
    : d_casClient(casClient)
{
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir == nullptr || tmpdir[0] == '\0') {
        tmpdir = "/tmp";
    }
    this->d_path = tmpdir + std::string("/buildboxrunXXXXXX");
    char *tempdir = mkdtemp(&this->d_path[0]);
    BUILDBOX_LOG_DEBUG("Downloading to " << std::string(tempdir));
    this->downloadDirectory(digest, tempdir);
}

FallbackStagedDirectory::~FallbackStagedDirectory()
{
    const char *argv[] = {"rm", "-rf", this->d_path.c_str(), nullptr};
    const auto pid = fork();
    if (pid == -1) {
        BUILDBOX_LOG_ERROR(
            "buildbox-run warning: failed to unstage directory: ");
        return;
    }
    else if (pid == 0) {
        execvp(argv[0], const_cast<char *const *>(argv));
        perror(argv[0]);
        _Exit(1);
    }
    int statLoc;
    if (waitpid(pid, &statLoc, 0) == -1) {
        BUILDBOX_LOG_ERROR(
            "buildbox-run warning: failed to unstage directory: ");
    }
    else if (WEXITSTATUS(statLoc) != 0) {
        BUILDBOX_LOG_ERROR(
            "buildbox-run warning: failed to unstage directory with "
            "exit code "
            << WEXITSTATUS(statLoc));
    }
}

// TODO: Rewrite captureFile to use file descriptors.
std::shared_ptr<OutputFile>
FallbackStagedDirectory::captureFile(const char *relativePath)
{
    BUILDBOX_LOG_DEBUG("Uploading " << relativePath);
    std::shared_ptr<OutputFile> result(new OutputFile());
    std::string file = this->d_path + std::string("/") + relativePath;
    // need to use O_RDWR or else wont throw error on directory
    int fd = open(file.c_str(), O_RDWR);
    if (fd == -1) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            return std::shared_ptr<OutputFile>();
        }
        throw std::system_error(errno, std::system_category());
    }
    try {
        result->set_path(relativePath);
        *(result->mutable_digest()) = CASHash::hash(fd);
        this->d_casClient->upload(fd, result->digest());
        result->set_is_executable(FileUtils::is_executable(file.c_str()));
    }
    catch (...) {
        close(fd);
        throw;
    }

    close(fd);
    return result;
}

Directory
FallbackStagedDirectory::uploadDirectoryRecursively(Tree *tree,
                                                    const char *relativePath)
{
    BUILDBOX_LOG_DEBUG("Uploading directory " << relativePath);
    std::map<std::string, FileNode> files;
    std::map<std::string, DirectoryNode> directories;
    struct dirent **direntList;

    int n = scandir(relativePath, &direntList, NULL, NULL);
    if (n == -1) {
        // On error we don't need to free direntList
        throw std::system_error(errno, std::system_category());
    }
    std::vector<std::string> nameList;
    for (int i = n - 1; i >= 0; i--) {
        nameList.push_back(direntList[i]->d_name);
        free(direntList[i]);
    }
    free(direntList);
    for (const std::string entry : nameList) {
        std::string entryRelativePath =
            std::string(relativePath) + "/" + entry;

        if (strcmp(entry.c_str(), ".") == 0 ||
            strcmp(entry.c_str(), "..") == 0) {
            // Skip "." and ".."
            continue;
        }

        // Check if the path is a file.
        auto outputFile = this->captureFile(entryRelativePath.c_str());
        if (outputFile) {
            FileNode fileNode;
            fileNode.set_name(entry);
            *(fileNode.mutable_digest()) = outputFile->digest();
            fileNode.set_is_executable(outputFile->is_executable());
            files[entry] = fileNode;
            continue;
        }

        // Try uploading the path as a directory.
        auto directory =
            uploadDirectoryRecursively(tree, entryRelativePath.c_str());
        DirectoryNode directoryNode;
        directoryNode.set_name(entry);
        *(directoryNode.mutable_digest()) =
            this->d_casClient->uploadMessage<Directory>(directory);
        *(tree->add_children()) = directory;
        directories[entry] = directoryNode;
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

std::shared_ptr<OutputDirectory>
FallbackStagedDirectory::captureDirectory(const char *relativePath)
{
    Tree tree;

    std::string dirPath = this->d_path;
    // using AT_FDCWD as placeholder. Since dirPath is absolute, openAT will
    // ignore AT_FDCWD
    const int dirFD = openat(AT_FDCWD, dirPath.c_str(), O_RDONLY);
    if (dirFD == -1) {
        if (errno == EACCES || errno == ENOTDIR || errno == ENOENT) {
            return std::shared_ptr<OutputDirectory>();
        }
        throw std::system_error(errno, std::system_category());
    }
    *(tree.mutable_root()) =
        this->uploadDirectoryRecursively(&tree, relativePath);

    std::shared_ptr<OutputDirectory> result(new OutputDirectory());
    result->set_path(relativePath);
    *(result->mutable_tree_digest()) =
        this->d_casClient->uploadMessage<Tree>(tree);
    return result;
}

void FallbackStagedDirectory::downloadFile(const Digest &digest,
                                           bool executable, const char *path)
{
    BUILDBOX_LOG_DEBUG("Downloading " << path);
    int fd =
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
                                                const char *path)
{
    BUILDBOX_LOG_DEBUG("Downloading directory " << path);
    Directory directory = this->d_casClient->fetchMessage<Directory>(digest);
    for (const auto filenode : directory.files()) {
        const std::string name = path + std::string("/") + filenode.name();
        this->downloadFile(filenode.digest(), filenode.is_executable(),
                           name.c_str());
    }
    for (const auto directoryNode : directory.directories()) {
        const std::string name =
            path + std::string("/") + directoryNode.name();
        if (mkdir(name.c_str(), 0777) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        this->downloadDirectory(directoryNode.digest(), name.c_str());
    }
    // TODO symlinks?
}

} // namespace buildboxcommon
