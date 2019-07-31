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

#include <buildboxcommon_logging.h>
#include <buildboxcommon_temporaryfile.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <libgen.h>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace buildboxcommon {

bool FileUtils::is_regular_file(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return S_ISREG(statResult.st_mode);
    }
    return false;
}

bool FileUtils::is_directory(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return S_ISDIR(statResult.st_mode);
    }
    return false;
}

bool FileUtils::directory_is_empty(const char *path)
{
    DIR *dir_stream = opendir(path);
    if (dir_stream == nullptr) {
        throw std::system_error(errno, std::system_category());
    }

    struct dirent *dir_entry;
    errno = 0;
    while ((dir_entry = readdir(dir_stream)) != nullptr) {
        const std::string entry_name = dir_entry->d_name;
        if (entry_name != "." && entry_name != "..") {
            return false;
        }
    }
    const auto readdir_status = errno; // (`nullptr` can be returned on errors)

    if (closedir(dir_stream) != 0) {
        BUILDBOX_LOG_ERROR("Could not closedir() " << path << ": "
                                                   << std::strerror(errno));
    }

    if (readdir_status != 0) {
        throw std::system_error(errno, std::system_category());
    }

    return true;
}

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
    delete_recursively(path, true);
}

void FileUtils::clear_directory(const char *path)
{
    delete_recursively(path, false);
}

bool FileUtils::is_executable(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return statResult.st_mode & S_IXUSR;
    }
    return false;
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

std::string FileUtils::get_file_contents(const char *path)
{
    std::ifstream file_stream(path, std::ios::in | std::ios::binary);
    if (file_stream.bad() || file_stream.fail()) {
        throw std::runtime_error("Failed to open file " + std::string(path) +
                                 ": " + std::strerror(errno));
    }

    std::ostringstream file_stringstream;
    file_stringstream << file_stream.rdbuf();
    if (file_stream.bad()) {
        throw std::runtime_error("Failed to read file " + std::string(path) +
                                 ": " + std::strerror(errno));
    }
    // (`std::stringstream::failbit` is set if the opened file reached EOF.
    // That is not an error: we can read empty files.)

    return file_stringstream.str();
}

int FileUtils::write_file_atomically(const std::string &path,
                                     const std::string &data, mode_t mode,
                                     const std::string &intermediate_directory,
                                     const std::string &prefix)
{
    std::string temporary_directory;
    if (!intermediate_directory.empty()) {
        temporary_directory = intermediate_directory;
    }
    else {
        // If no intermediate directory is specified, we use the parent
        // directory in `path`.

        // `dirname()` modifies its input, so we give it a copy.
        std::vector<char> output_path(path.cbegin(), path.end());
        const char *parent_directory = dirname(&output_path[0]);
        temporary_directory = std::string(parent_directory);
    }

    buildboxcommon::TemporaryFile temp_file(temporary_directory.c_str(),
                                            prefix.c_str(), mode);
    // `temp_file`'s destructor will `unlink()` the created file, removing it
    // from the temporary directory.

    const std::string temp_filename = temp_file.name();

    // Writing the data to it:
    std::ofstream file(temp_filename, std::fstream::binary);
    file << data;
    file.close();

    if (!file.good()) {
        const std::string error_message = "Failed writing to temporary file " +
                                          temp_filename + ": " +
                                          strerror(errno);
        BUILDBOX_LOG_ERROR(error_message);
        throw std::system_error(errno, std::system_category());
    }

    // Creating a hard link (atomic operation) from the destination to the
    // temporary file:
    if (link(temp_filename.c_str(), path.c_str()) == 0) {
        return 0;
    }
    return errno;
}

void FileUtils::delete_recursively(const char *path,
                                   const bool delete_root_directory)
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

        if (is_directory(entryPath.c_str())) {
            delete_recursively(entryPath.c_str(), true);
        }
        else {
            unlink(entryPath.c_str());
        }
    }

    closedir(dirStream);

    if (delete_root_directory && rmdir(path) == -1) {
        throw std::system_error(errno, std::system_category());
    }
    // (The value of `delete_root_directory` is only considered for the first
    // call of this function, the recursive calls will all invoked with it set
    // to true.)
}

std::string FileUtils::make_path_absolute(const std::string &path,
                                          const std::string &cwd)
{
    if (cwd.empty() || cwd.front() != '/') {
        throw std::runtime_error("`cwd must be an absolute path");
    }

    const std::string full_path = cwd + '/' + path;
    std::string normalized_path = FileUtils::normalize_path(full_path.c_str());

    /* normalize_path removes trailing slashes, so let's preserve them here */
    if (path.back() == '/' && normalized_path.back() != '/') {
        normalized_path.push_back('/');
    }
    return normalized_path;
}

std::string FileUtils::normalize_path(const char *path)
{
    std::vector<std::string> segments;

    const bool global = path[0] == '/';
    while (path[0] != '\0') {
        const char *slash = strchr(path, '/');
        std::string segment;
        if (slash == nullptr) {
            segment = std::string(path);
        }
        else {
            segment = std::string(path, size_t(slash - path));
        }
        if (segment == ".." && !segments.empty() && segments.back() != "..") {
            segments.pop_back();
        }
        else if (segment != "." && segment != "") {
            segments.push_back(segment);
        }
        if (slash == nullptr) {
            break;
        }
        else {
            path = slash + 1;
        }
    }

    std::string result(global ? "/" : "");
    if (segments.size() > 0) {
        for (const auto &segment : segments) {
            result += segment + "/";
        }
        result.pop_back();
    }
    return result;
}

} // namespace buildboxcommon
