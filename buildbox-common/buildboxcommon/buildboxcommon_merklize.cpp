// Copyright 2019 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <buildboxcommon_merklize.h>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_timeutils.h>

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

namespace {

Digest hashFile(int fd) { return CASHash::hash(fd); }

} // namespace

File::File(const char *path,
           const std::vector<std::string> &capture_properties)
    : File(path, hashFile, capture_properties)
{
}

File::File(const char *path, const FileDigestFunction &fileDigestFunc,
           const std::vector<std::string> &capture_properties)
    : File(AT_FDCWD, path, fileDigestFunc, capture_properties)
{
}

File::File(int dirfd, const char *path,
           const FileDigestFunction &fileDigestFunc,
           const std::vector<std::string> &capture_properties)
{
    const int fd = openat(dirfd, path, O_RDONLY);
    if (fd < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed to open path \"" << path << "\"");
    }
    try {
        d_executable = FileUtils::isExecutable(fd);
        d_digest = fileDigestFunc(fd);
        for (const std::string &property : capture_properties) {
            if (property == "mtime") {
                d_mtime = FileUtils::getFileMtime(fd);
                d_mtime_set = true;
            }
        }
        close(fd);
    }
    catch (...) {
        close(fd);
        throw;
    }
}

FileNode File::to_filenode(const std::string &name) const
{
    FileNode result;
    result.set_name(name);
    *result.mutable_digest() = d_digest;
    result.set_is_executable(d_executable);
    if (d_mtime_set) {
        auto node_properties = result.mutable_node_properties();
        node_properties->mutable_mtime()->CopyFrom(
            TimeUtils::make_timestamp(d_mtime));
    }
    return result;
}

void NestedDirectory::add(const File &file, const char *relativePath)
{
    const char *slash = strchr(relativePath, '/');
    if (slash) {
        const std::string subdirKey(relativePath,
                                    static_cast<size_t>(slash - relativePath));
        if (subdirKey.empty()) {
            this->add(file, slash + 1);
        }
        else {
            (*d_subdirs)[subdirKey].add(file, slash + 1);
        }
    }
    else {
        d_files[std::string(relativePath)] = file;
    }
}

void NestedDirectory::addSymlink(const std::string &target,
                                 const char *relativePath)
{
    const char *slash = strchr(relativePath, '/');
    if (slash) {
        const std::string subdirKey(relativePath,
                                    static_cast<size_t>(slash - relativePath));
        if (subdirKey.empty()) {
            this->addSymlink(target, slash + 1);
        }
        else {
            (*d_subdirs)[subdirKey].addSymlink(target, slash + 1);
        }
    }
    else {
        d_symlinks[std::string(relativePath)] = target;
    }
}

void NestedDirectory::addDirectory(const char *directory)
{
    // A forward slash by itself is not a valid input directory
    if (strcmp(directory, "/") == 0) {
        return;
    }
    const char *slash = strchr(directory, '/');
    if (slash) {
        const std::string subdirKey(directory,
                                    static_cast<size_t>(slash - directory));
        if (subdirKey.empty()) {
            this->addDirectory(slash + 1);
        }
        else {
            (*d_subdirs)[subdirKey].addDirectory(slash + 1);
        }
    }
    else {
        if ((*d_subdirs).count(directory) == 0) {
            (*d_subdirs)[directory] = NestedDirectory();
        }
    }
}

Digest NestedDirectory::to_digest(digest_string_map *digestMap) const
{
    // The 'd_files' and 'd_subdirs' maps make sure everything is sorted by
    // name thus the iterators will iterate lexicographically

    Directory directoryMessage;
    for (const auto &fileIter : d_files) {
        *directoryMessage.add_files() =
            fileIter.second.to_filenode(fileIter.first);
    }
    for (const auto &symlinkIter : d_symlinks) {
        SymlinkNode symlinkNode;
        symlinkNode.set_name(symlinkIter.first);
        symlinkNode.set_target(symlinkIter.second);
        *directoryMessage.add_symlinks() = symlinkNode;
    }
    for (const auto &subdirIter : *d_subdirs) {
        auto subdirNode = directoryMessage.add_directories();
        subdirNode->set_name(subdirIter.first);
        auto subdirDigest = subdirIter.second.to_digest(digestMap);
        *subdirNode->mutable_digest() = subdirDigest;
    }
    auto blob = directoryMessage.SerializeAsString();
    auto digest = make_digest(blob);
    if (digestMap != nullptr) {
        (*digestMap)[digest] = blob;
    }
    return digest;
}

Tree NestedDirectory::to_tree() const
{
    Tree result;
    auto root = result.mutable_root();
    for (const auto &fileIter : d_files) {
        *root->add_files() = fileIter.second.to_filenode(fileIter.first);
    }
    for (const auto &symlinkIter : d_symlinks) {
        SymlinkNode symlinkNode;
        symlinkNode.set_name(symlinkIter.first);
        symlinkNode.set_target(symlinkIter.second);
        *root->add_symlinks() = symlinkNode;
    }
    for (const auto &subdirIter : *d_subdirs) {
        auto subtree = subdirIter.second.to_tree();
        result.mutable_children()->MergeFrom(subtree.children());
        *result.add_children() = subtree.root();
        auto subdirNode = root->add_directories();
        subdirNode->set_name(subdirIter.first);
        *subdirNode->mutable_digest() = make_digest(subtree.root());
    }
    return result;
}

Digest make_digest(const std::string &blob) { return CASHash::hash(blob); }

static NestedDirectory
make_nesteddirectory(int basedirfd, const std::string prefix, const char *path,
                     const FileDigestFunction &fileDigestFunc,
                     digest_string_map *fileMap,
                     const std::vector<std::string> &capture_properties);

NestedDirectory
make_nesteddirectory(const char *path, digest_string_map *fileMap,
                     const std::vector<std::string> &capture_properties)
{
    return make_nesteddirectory(path, hashFile, fileMap, capture_properties);
}

NestedDirectory
make_nesteddirectory(const char *path,
                     const FileDigestFunction &fileDigestFunc,
                     digest_string_map *fileMap,
                     const std::vector<std::string> &capture_properties)
{
    return make_nesteddirectory(AT_FDCWD, "", path, fileDigestFunc, fileMap,
                                capture_properties);
}

NestedDirectory
make_nesteddirectory(int basedirfd, const std::string prefix, const char *path,
                     const FileDigestFunction &fileDigestFunc,
                     digest_string_map *fileMap,
                     const std::vector<std::string> &capture_properties)
{
    NestedDirectory result;
    const int dirfd = openat(basedirfd, path, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed to open path \"" << path << "\"");
    }
    const auto dir = fdopendir(dirfd);
    if (dir == nullptr) {
        close(dirfd);
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed to open path \"" << path << "\"");
    }

    const std::string newprefix(prefix + path + "/");
    for (auto dirent = readdir(dir); dirent != nullptr;
         dirent = readdir(dir)) {
        if (strcmp(dirent->d_name, ".") == 0 ||
            strcmp(dirent->d_name, "..") == 0) {
            continue;
        }

        const std::string entityName(dirent->d_name);
        const std::string entityPath = newprefix + entityName;

        struct stat statResult;
        if (fstatat(dirfd, dirent->d_name, &statResult, AT_SYMLINK_NOFOLLOW) !=
            0) {
            continue;
        }

        if (S_ISDIR(statResult.st_mode)) {
            (*result.d_subdirs)[entityName] = make_nesteddirectory(
                dirfd, newprefix, dirent->d_name, fileDigestFunc, fileMap,
                capture_properties);
        }
        else if (S_ISREG(statResult.st_mode)) {
            const File file(dirfd, dirent->d_name, fileDigestFunc,
                            capture_properties);
            result.d_files[entityName] = file;

            if (fileMap != nullptr) {
                (*fileMap)[file.d_digest] = entityPath;
            }
        }
        else if (S_ISLNK(statResult.st_mode)) {
            std::string target(static_cast<size_t>(statResult.st_size), '\0');

            if (readlinkat(dirfd, dirent->d_name, &target[0], target.size()) <
                0) {
                closedir(dir);
                BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                    std::system_error, errno, std::system_category,
                    "Error reading symlink at \""
                        << entityPath
                        << "\", st_size = " << statResult.st_size);
            }
            result.d_symlinks[entityName] = target;
        }
    }

    // This will implicitly close `dirfd`.
    closedir(dir);
    return result;
}

} // namespace buildboxcommon
