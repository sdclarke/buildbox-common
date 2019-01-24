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

#include <cstring>
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
    this->downloadDirectory(digest, mkdtemp(&this->d_path[0]));
}

FallbackStagedDirectory::~FallbackStagedDirectory()
{
    int argc = 3;
    const char *argv[] = {"rm", "-rf", this->d_path.c_str(), nullptr};
    const auto pid = fork();
    if (pid == -1) {
        perror("buildbox-run warning: failed to unstage directory: ");
        return;
    }
    else if (pid == 0) {
        execvp(argv[0], const_cast<char *const *>(argv));
        perror(argv[0]);
        _Exit(1);
    }
    int statLoc;
    if (waitpid(pid, &statLoc, 0) == -1) {
        perror("buildbox-run warning: failed to unstage directory: ");
    }
    else if (WEXITSTATUS(statLoc) != 0) {
        std::cerr << "buildbox-run warning: failed to unstage directory with "
                     "exit code "
                  << WEXITSTATUS(statLoc) << std::endl;
    }
}

std::shared_ptr<OutputFile>
FallbackStagedDirectory::captureFile(const char *relativePath)
{
    std::string file = this->d_path + std::string("/") + relativePath;
    int fd = open(file.c_str(), O_RDONLY);
    if (fd == -1) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            return std::shared_ptr<OutputFile>();
        }
        throw std::system_error(errno, std::system_category());
    }
    std::shared_ptr<OutputFile> result(new OutputFile());
    result->set_path(relativePath);
    *(result->mutable_digest()) = CASHash::hash(fd);
    this->d_casClient->upload(fd, result->digest());

    // Check if the file is executable
    struct stat statResult;
    if (stat(file.c_str(), &statResult) == 0) {
        result->set_is_executable((statResult.st_mode & S_IXUSR) != 0);
    }

    close(fd);
    return result;
}

Directory
FallbackStagedDirectory::uploadDirectoryRecursively(Tree *tree, DIR *dirStream,
                                                    const char *relativePath)
{
    std::map<std::string, FileNode> files;
    std::map<std::string, DirectoryNode> directories;
    // TODO symlinks?
    for (auto entry = readdir(dirStream); entry != nullptr;
         entry = readdir(dirStream)) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            // Skip "." and ".."
            continue;
        }
        std::string entryRelativePath =
            relativePath + std::string("/") + entry->d_name;

        // Check if the path is a file.
        auto outputFile = this->captureFile(entryRelativePath.c_str());
        if (outputFile) {
            FileNode fileNode;
            fileNode.set_name(entry->d_name);
            *(fileNode.mutable_digest()) = outputFile->digest();
            fileNode.set_is_executable(outputFile->is_executable());
            files[entry->d_name] = fileNode;
            continue;
        }

        // Try uploading the path as a directory.
        std::string entryAbsolutePath = this->d_path + "/" + entryRelativePath;
        DIR *childDirStream = opendir(entryAbsolutePath.c_str());
        if (childDirStream != nullptr) {
            auto directory = uploadDirectoryRecursively(
                tree, childDirStream, entryRelativePath.c_str());
            DirectoryNode directoryNode;
            directoryNode.set_name(entry->d_name);
            *(directoryNode.mutable_digest()) =
                this->d_casClient->uploadMessage(directory);
            *(tree->add_children()) = directory;
            directories[entry->d_name] = directoryNode;
            continue;
        }
    }
    closedir(dirStream);
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

    std::string dirPath = this->d_path + std::string("/") + relativePath;
    DIR *dirStream = opendir(dirPath.c_str());
    if (dirStream == nullptr) {
        if (errno == EACCES || errno == ENOTDIR || errno == ENOENT) {
            return std::shared_ptr<OutputDirectory>();
        }
        throw std::system_error(errno, std::system_category());
    }

    *(tree.mutable_root()) =
        this->uploadDirectoryRecursively(&tree, dirStream, relativePath);

    std::shared_ptr<OutputDirectory> result(new OutputDirectory());
    result->set_path(relativePath);
    *(result->mutable_tree_digest()) = this->d_casClient->uploadMessage(tree);
    return result;
}

void FallbackStagedDirectory::downloadFile(const Digest &digest,
                                           bool executable, const char *path)
{
    int fd =
        open(path, O_CREAT | O_WRONLY | O_TRUNC, executable ? 0755 : 0644);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    this->d_casClient->download(fd, digest);
    close(fd);
}

void FallbackStagedDirectory::downloadDirectory(const Digest &digest,
                                                const char *path)
{
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
