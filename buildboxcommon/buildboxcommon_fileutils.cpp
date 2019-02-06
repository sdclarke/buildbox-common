// Copyright 2018 Bloomberg Finance L.P
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

#include <buildboxcommon_fileutils.h>

#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

const char *FileUtilsDefaults::DEFAULT_TMP_PREFIX = "buildbox";

void FileUtils::create_directory(const char *path)
{
    if (mkdir(path, 0777) != 0) {
        if (errno == EEXIST) {
            // The directory already exists, so return.
            return;
        }
        else if (errno == ENOENT) {
            auto lastSlash = strrchr(path, '/');
            if (lastSlash == nullptr) {
                throw std::system_error(errno, std::system_category());
            }
            std::string parent(path, lastSlash - path);
            create_directory(parent.c_str());
            if (mkdir(path, 0777) != 0) {
                throw std::system_error(errno, std::system_category());
            }
        }
        else {
            throw std::system_error(errno, std::system_category());
        }
    }
}

void FileUtils::delete_directory(const char *path)
{
    DIR *dirStream = opendir(path);
    if (dirStream == nullptr) {
        throw std::system_error(errno, std::system_category());
    }

    for (auto entry = readdir(dirStream); entry != nullptr;
         entry = readdir(dirStream)) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            // Skip "." and ".."
            continue;
        }

        std::string entryPath =
            std::string(path) + std::string("/") + entry->d_name;

        if (entry->d_type == DT_DIR) {
            DIR *entryStream = opendir(entryPath.c_str());
            if (dirStream != nullptr) {
                delete_directory(entryPath.c_str());
            }
        }
        else {
            unlink(entryPath.c_str());
        }
    }

    closedir(dirStream);

    if (rmdir(path) == -1) {
        throw std::system_error(errno, std::system_category());
    }
}

bool FileUtils::is_executable(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return statResult.st_mode & S_IXUSR;
    }
    throw std::system_error(errno, std::system_category());
}

void FileUtils::make_executable(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        if (chmod(path, statResult.st_mode | S_IXUSR | S_IXGRP | S_IXOTH) ==
            0) {
            return;
        }
    }
    throw std::system_error(errno, std::system_category());
}

} // namespace buildboxcommon
