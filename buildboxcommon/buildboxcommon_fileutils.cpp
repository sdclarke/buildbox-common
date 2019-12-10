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

#include <buildboxcommon_direntwrapper.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_temporaryfile.h>
#include <buildboxcommon_timeutils.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <libgen.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <vector>

#if __APPLE__
#define st_mtim st_mtimespec
#define st_atim st_atimespec
#endif

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

bool FileUtils::is_directory(int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) == 0) {
        return S_ISDIR(statResult.st_mode);
    }
    return false;
}

bool FileUtils::directory_is_empty(const char *path)
{
    DirentWrapper root(path);

    // DirentWrapper's entry will point to an entry in the directory (not
    // including "." and ".."). If the entry isn't nullptr, then the directory
    // can't be empty, therefore return false.
    return (root.entry() == nullptr);
}

void FileUtils::create_directory(const char *path, mode_t mode)
{
    // Normalize path first as the parent directory creation logic below
    // can't handle paths with '..' components.
    const std::string normalizedStr = normalize_path(path);
    const char *normalizeStrPtr = normalizedStr.c_str();
    if (mkdir(normalizeStrPtr, mode) != 0) {
        if (errno == EEXIST) {
            // The directory already exists, so return.
            return;
        }
        else if (errno == ENOENT) {
            const char *lastSlash = strrchr(normalizeStrPtr, '/');
            if (lastSlash == nullptr) {
                throw std::system_error(errno, std::system_category());
            }
            const std::string parent(normalizeStrPtr,
                                     static_cast<unsigned long>(std::distance(
                                         normalizeStrPtr, lastSlash)));
            create_directory(parent.c_str());
            if (mkdir(normalizeStrPtr, mode) != 0) {
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

bool FileUtils::is_executable(int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) == 0) {
        return statResult.st_mode & S_IXUSR;
    }
    return false;
}

struct stat FileUtils::get_file_stat(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) != 0) {
        BUILDBOX_LOG_ERROR("Failed to get file stats at " << path << ".");
        throw std::system_error(errno, std::system_category());
    }
    return statResult;
}

struct stat FileUtils::get_file_stat(const int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) != 0) {
        BUILDBOX_LOG_ERROR("Failed to get file stats.");
        throw std::system_error(errno, std::system_category());
    }
    return statResult;
}

std::chrono::system_clock::time_point
FileUtils::get_file_mtime(const char *path)
{
    struct stat statResult = get_file_stat(path);
    return FileUtils::get_mtime_timepoint(statResult);
}

std::chrono::system_clock::time_point FileUtils::get_file_mtime(const int fd)
{
    struct stat statResult = get_file_stat(fd);
    return FileUtils::get_mtime_timepoint(statResult);
}

std::chrono::system_clock::time_point
FileUtils::get_mtime_timepoint(struct stat &result)
{
    const std::chrono::system_clock::time_point timepoint =
        std::chrono::system_clock::from_time_t(result.st_mtim.tv_sec) +
        std::chrono::microseconds{result.st_mtim.tv_nsec / 1000};
    return timepoint;
}

void FileUtils::set_file_mtime(
    const int fd, const std::chrono::system_clock::time_point timepoint)
{
    const struct timespec new_mtime = TimeUtils::make_timespec(timepoint);
    const struct stat stat_result = FileUtils::get_file_stat(fd);
    // AIX stat.h defines st_timespec_t as the return of stat().st_atim
    const struct timespec atime = {
        stat_result.st_atim.tv_sec,
        static_cast<long>(stat_result.st_atim.tv_nsec)};
    const struct timespec times[2] = {atime, new_mtime};
    if (futimens(fd, times) == 0) {
        return;
    }
    BUILDBOX_LOG_ERROR("Failed to set file mtime: " << std::strerror(errno));
    throw std::system_error(errno, std::system_category());
}

void FileUtils::set_file_mtime(
    const char *path, const std::chrono::system_clock::time_point timepoint)
{
    const struct timespec new_mtime = TimeUtils::make_timespec(timepoint);
    const struct stat stat_result = FileUtils::get_file_stat(path);
    // AIX stat.h defines st_timespec_t as the return of stat().st_atim
    const struct timespec atime = {
        stat_result.st_atim.tv_sec,
        static_cast<long>(stat_result.st_atim.tv_nsec)};
    const struct timespec times[2] = {atime, new_mtime};
    if (utimensat(AT_FDCWD, path, times, 0) == 0) {
        return;
    }
    BUILDBOX_LOG_ERROR("Failed to set file "
                       << path << " mtime: " << std::strerror(errno));
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

void FileUtils::copy_file(const char *src_path, const char *dest_path)
{
    ssize_t rdsize, wrsize;
    int err = 0;
    const mode_t mode = FileUtils::get_file_stat(src_path).st_mode;

    int src = open(src_path, O_RDONLY, mode);
    if (src == -1) {
        err = errno;
        BUILDBOX_LOG_ERROR("Failed to open file at " << src_path);
    }

    int dest = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest == -1) {
        err = errno;
        BUILDBOX_LOG_ERROR("Failed to open file at " << dest_path);
    }

    if (err == 0) {
        const size_t bufsize = 65536;
        char *buf = new char[bufsize];
        while ((rdsize = read(src, buf, bufsize)) > 0) {
            wrsize = write(dest, buf, rdsize);
            if (wrsize != rdsize) {
                err = EIO;
                BUILDBOX_LOG_ERROR("Failed to write to file at " << dest_path);
                break;
            }
        }
        delete[] buf;

        if (rdsize == -1) {
            err = errno;
            BUILDBOX_LOG_ERROR("Failed to read file at " << src_path);
        }

        if (fchmod(dest, mode) != 0) {
            err = errno;
            BUILDBOX_LOG_ERROR("Failed to set mode of file at " << dest_path);
        }
    }

    if (close(src) != 0) {
        err = errno;
        BUILDBOX_LOG_ERROR("Failed to close file at " << src_path);
    }

    if (close(dest) != 0) {
        err = errno;
        BUILDBOX_LOG_ERROR("Failed to close file at " << dest_path);
    }

    if (err != 0) {
        if (unlink(dest_path) != 0) {
            err = errno;
            BUILDBOX_LOG_ERROR("Failed to remove file at " << dest_path);
        }
        throw std::system_error(err, std::system_category());
    }
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
        std::vector<char> output_path(path.cbegin(), path.cend());
        output_path.push_back('\0');
        const char *parent_directory = dirname(&output_path[0]);

        if (parent_directory == nullptr) {
            throw std::system_error(
                errno, std::system_category(),
                "Could not determine intermediate "
                "directory with `dirname(3)` for atomic write to " +
                    path);
        }
        temporary_directory = std::string(parent_directory);
    }

    std::unique_ptr<buildboxcommon::TemporaryFile> temp_file = nullptr;
    try {
        temp_file = std::make_unique<buildboxcommon::TemporaryFile>(
            temporary_directory.c_str(), prefix.c_str(), mode);
    }
    catch (const std::system_error &e) {
        BUILDBOX_LOG_ERROR("Error creating intermediate file in " +
                           temporary_directory + " for atomic write to " +
                           path + ": " + std::string(e.what()));
        throw e;
    }

    // `temp_file`'s destructor will `unlink()` the created file, removing
    // it from the temporary directory.

    const std::string temp_filename = temp_file->name();

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
    DirentWrapper root(path);

    DirectoryTraversalFnPtr rmdir_func = [](const char *dir_path, int fd) {
        std::string dir_basename(dir_path);
        if (fd != -1) {
            dir_basename = FileUtils::path_basename(dir_path);
            if (dir_basename.empty()) {
                return;
            }
        }
        // unlinkat will disregard the file descriptor and call
        // rmdir/unlink on the path depending on the entity
        // type(file/directory).
        //
        // For deletion using the file descriptor, the path must be
        // relative to the directory the file descriptor points to.
        if (unlinkat(fd, dir_basename.c_str(), AT_REMOVEDIR) == -1) {
            int errsv = errno;
            BUILDBOX_LOG_ERROR("Error removing directory: "
                               << "[" << dir_path << "]"
                               << " with error: " << strerror(errsv));
            throw std::system_error(errno, std::system_category());
        }
    };

    DirectoryTraversalFnPtr unlink_func = [](const char *file_path = nullptr,
                                             int fd = 0) {
        if (unlinkat(fd, file_path, 0) == -1) {
            int errsv = errno;
            BUILDBOX_LOG_ERROR("Error removing file: "
                               << "[" << file_path << "]"
                               << " with error: " << strerror(errsv));
            throw std::system_error(errsv, std::system_category());
        }
    };

    FileDescriptorTraverseAndApply(&root, rmdir_func, unlink_func,
                                   delete_root_directory, true);
}

void FileUtils::FileDescriptorTraverseAndApply(
    DirentWrapper *dir, DirectoryTraversalFnPtr dir_func,
    DirectoryTraversalFnPtr file_func, bool apply_to_root, bool pass_parent_fd)
{
    while (dir->entry() != nullptr) {
        if (dir->currentEntryIsDirectory()) {
            auto nextDir = dir->nextDir();
            FileDescriptorTraverseAndApply(&nextDir, dir_func, file_func, true,
                                           pass_parent_fd);
        }
        else {
            if (file_func != nullptr) {
                file_func(dir->entry()->d_name, dir->fd());
            }
        }
        dir->next();
    }
    if (apply_to_root && dir_func != nullptr) {
        if (pass_parent_fd) {
            dir_func(dir->path().c_str(), dir->pfd());
        }
        else {
            dir_func(dir->path().c_str(), dir->fd());
        }
    }
}

std::string FileUtils::make_path_absolute(const std::string &path,
                                          const std::string &cwd)
{
    if (cwd.empty() || cwd.front() != '/') {
        std::ostringstream os;
        os << "cwd must be an absolute path: [" << cwd << "]";
        const std::string err = os.str();
        BUILDBOX_LOG_ERROR(err);
        throw std::runtime_error(err);
    }

    const std::string full_path = cwd + '/' + path;
    std::string normalized_path = FileUtils::normalize_path(full_path.c_str());

    // normalize_path removes trailing slashes, so let's preserve them here
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

std::string FileUtils::path_basename(const char *path)
{
    std::string basename(path);

    // Check for root or empty
    if (basename.empty() || basename.size() == 1) {
        return "";
    }

    // Remove trailing slash.
    if (basename[basename.size() - 1] == '/') {
        basename = basename.substr(0, basename.rfind('/'));
    }

    auto pos = basename.rfind('/');

    if (pos == std::string::npos) {
        return "";
    }
    else {
        return basename.substr(pos + 1);
    }
}

} // namespace buildboxcommon
