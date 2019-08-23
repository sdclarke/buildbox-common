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
#include <buildboxcommon_fileutils.h>

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>

namespace buildboxcommon {

File::File(const char *path)
{
    d_executable = FileUtils::is_executable(path);
    d_digest = make_digest(FileUtils::get_file_contents(path));
    return;
}

FileNode File::to_filenode(const std::string &name) const
{
    FileNode result;
    result.set_name(name);
    *result.mutable_digest() = d_digest;
    result.set_is_executable(d_executable);
    return result;
}

void NestedDirectory::add(File file, const char *relativePath)
{
    const char *slash = strchr(relativePath, '/');
    if (slash) {
        const std::string subdirKey(relativePath, slash - relativePath);
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

void NestedDirectory::addDirectory(const char *directory)
{
    // A forward slash by itself is not a valid input directory
    if (strcmp(directory, "/") == 0) {
        return;
    }
    const char *slash = strchr(directory, '/');
    if (slash) {
        const std::string subdirKey(directory, slash - directory);
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

NestedDirectory make_nesteddirectory(const char *path,
                                     digest_string_map *fileMap)
{
    NestedDirectory result;
    auto dir = opendir(path);
    if (dir == NULL) {
        throw std::system_error(errno, std::system_category());
    }

    std::string pathString(path);
    for (auto dirent = readdir(dir); dirent != nullptr;
         dirent = readdir(dir)) {
        if (strcmp(dirent->d_name, ".") == 0 ||
            strcmp(dirent->d_name, "..") == 0) {
            continue;
        }
        std::string entityName(dirent->d_name);
        std::string entityPath = pathString + "/" + entityName;

        struct stat statResult;
        if (stat(entityPath.c_str(), &statResult) != 0) {
            continue;
        }
        if (S_ISDIR(statResult.st_mode)) {
            (*result.d_subdirs)[entityName] =
                make_nesteddirectory(entityPath.c_str(), fileMap);
        }
        else {
            File file(entityPath.c_str());
            result.d_files[entityName] = file;
            if (fileMap != nullptr) {
                (*fileMap)[file.d_digest] = entityPath;
            }
        }
    }
    closedir(dir);
    return result;
}

} // namespace buildboxcommon
