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

#include <buildboxcommon_direntwrapper.h>
#include <buildboxcommon_exception.h>
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

bool FileUtils::isRegularFile(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return S_ISREG(statResult.st_mode);
    }
    return false;
}

bool FileUtils::isDirectory(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return S_ISDIR(statResult.st_mode);
    }
    return false;
}

bool FileUtils::isDirectory(int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) == 0) {
        return S_ISDIR(statResult.st_mode);
    }
    return false;
}

bool FileUtils::isSymlink(const char *path)
{
    struct stat statResult;
    if (lstat(path, &statResult) == 0) {
        return S_ISLNK(statResult.st_mode);
    }
    return false;
}

bool FileUtils::directoryIsEmpty(const char *path)
{
    DirentWrapper root(path);

    // DirentWrapper's entry will point to an entry in the directory (not
    // including "." and ".."). If the entry isn't nullptr, then the directory
    // can't be empty, therefore return false.
    return (root.entry() == nullptr);
}

void FileUtils::createDirectory(const char *path, mode_t mode)
{
    // Normalize path first as the parent directory creation logic in
    // `createDirectoriesInPath()` can't handle paths with '..' components.
    const std::string normalized_path = normalizePath(path);
    createDirectoriesInPath(normalized_path, mode);
}

void FileUtils::createDirectoriesInPath(const std::string &path,
                                        const mode_t mode)
{
    // Attempt to create the directory:
    const auto mkdir_status = mkdir(path.c_str(), mode);
    const auto mkdir_error = errno;

    const auto log_and_throw = [&path](const int errno_value) {
        BUILDBOX_LOG_ERROR("Could not create directory [" + path +
                           "]: " + strerror(errno_value))
        throw std::system_error(errno_value, std::system_category());
    };

    if (mkdir_status == 0 || mkdir_error == EEXIST) {
        return; // Directory was succesfully created or already exists, done.
    }
    if (mkdir_error != ENOENT) { // Something went wrong, aborting.
        log_and_throw(mkdir_error);
    }

    // `mkdir_error == ENOENT` => Some portion of the path does not exist yet.
    // We'll recursively create the parent directory and try again:
    const std::string parent_path = path.substr(0, path.rfind('/'));
    createDirectoriesInPath(parent_path.c_str(), mode);

    // Now that all the parent directories exist, we create the last directory:
    if (mkdir(path.c_str(), mode) != 0) {
        log_and_throw(errno);
    }
}

void FileUtils::deleteDirectory(const char *path)
{
    deleteRecursively(path, true);
}

void FileUtils::clearDirectory(const char *path)
{
    deleteRecursively(path, false);
}

bool FileUtils::isExecutable(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        return statResult.st_mode & S_IXUSR;
    }
    return false;
}

bool FileUtils::isExecutable(int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) == 0) {
        return statResult.st_mode & S_IXUSR;
    }
    return false;
}

struct stat FileUtils::getFileStat(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) != 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed to get file stats at \"" << path << "\"");
    }
    return statResult;
}

struct stat FileUtils::getFileStat(const int fd)
{
    struct stat statResult;
    if (fstat(fd, &statResult) != 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed to get file stats for file descriptor " << fd);
    }
    return statResult;
}

std::chrono::system_clock::time_point FileUtils::getFileMtime(const char *path)
{
    struct stat statResult = getFileStat(path);
    return FileUtils::getMtimeTimepoint(statResult);
}

std::chrono::system_clock::time_point FileUtils::getFileMtime(const int fd)
{
    struct stat statResult = getFileStat(fd);
    return FileUtils::getMtimeTimepoint(statResult);
}

std::chrono::system_clock::time_point
FileUtils::getMtimeTimepoint(struct stat &result)
{
    const std::chrono::system_clock::time_point timepoint =
        std::chrono::system_clock::from_time_t(result.st_mtim.tv_sec) +
        std::chrono::microseconds{result.st_mtim.tv_nsec / 1000};
    return timepoint;
}

void FileUtils::setFileMtime(
    const int fd, const std::chrono::system_clock::time_point timepoint)
{
    const struct timespec new_mtime = TimeUtils::make_timespec(timepoint);
    const struct stat stat_result = FileUtils::getFileStat(fd);
    // AIX stat.h defines st_timespec_t as the return of stat().st_atim
    const struct timespec atime = {
        stat_result.st_atim.tv_sec,
        static_cast<long>(stat_result.st_atim.tv_nsec)};
    const struct timespec times[2] = {atime, new_mtime};
    if (futimens(fd, times) == 0) {
        return;
    }
    BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                          std::system_category,
                                          "Failed to set file mtime");
}

void FileUtils::setFileMtime(
    const char *path, const std::chrono::system_clock::time_point timepoint)
{
    const struct timespec new_mtime = TimeUtils::make_timespec(timepoint);
    const struct stat stat_result = FileUtils::getFileStat(path);
    // AIX stat.h defines st_timespec_t as the return of stat().st_atim
    const struct timespec atime = {
        stat_result.st_atim.tv_sec,
        static_cast<long>(stat_result.st_atim.tv_nsec)};
    const struct timespec times[2] = {atime, new_mtime};
    if (utimensat(AT_FDCWD, path, times, 0) == 0) {
        return;
    }
    BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
        std::system_error, errno, std::system_category,
        "Failed to set file \"" << path << "\" mtime");
}

void FileUtils::makeExecutable(const char *path)
{
    struct stat statResult;
    if (stat(path, &statResult) == 0) {
        if (chmod(path, statResult.st_mode | S_IXUSR | S_IXGRP | S_IXOTH) ==
            0) {
            return;
        }
    }
    BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
        std::system_error, errno, std::system_category,
        "Error in stat for path \"" << path << "\"");
}

std::string FileUtils::getFileContents(const char *path)
{
    std::ifstream file_stream(path, std::ios::in | std::ios::binary);
    if (file_stream.bad() || file_stream.fail()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Failed to open file \"" << path << "\"");
    }

    std::ostringstream file_stringstream;
    file_stringstream << file_stream.rdbuf();
    if (file_stream.bad()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Failed to read file \"" << path << "\"");
    }
    // (`std::stringstream::failbit` is set if the opened file reached EOF.
    // That is not an error: we can read empty files.)

    return file_stringstream.str();
}

void FileUtils::copyFile(const char *src_path, const char *dest_path)
{
    ssize_t rdsize, wrsize;
    int err = 0;
    const mode_t mode = FileUtils::getFileStat(src_path).st_mode;

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
            wrsize = write(dest, buf, static_cast<unsigned long>(rdsize));
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

int FileUtils::writeFileAtomically(const std::string &path,
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
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Could not determine intermediate "
                    << "directory with `dirname(3)` for atomic write to path "
                       "\""
                    << path << "\"");
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
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Failed writing to temporary file \"" << temp_filename << "\"");
    }

    // Creating a hard link (atomic operation) from the destination to the
    // temporary file:
    if (link(temp_filename.c_str(), path.c_str()) == 0) {
        return 0;
    }
    return errno;
}

void FileUtils::deleteRecursively(const char *path,
                                  const bool delete_root_directory)
{
    DirentWrapper root(path);

    DirectoryTraversalFnPtr rmdir_func = [](const char *dir_path, int fd) {
        std::string dir_basename(dir_path);
        if (fd != -1) {
            dir_basename = FileUtils::pathBasename(dir_path);
            if (dir_basename.empty()) {
                return;
            }
        }
        else {
            // The root directory has no parent directory file descriptor,
            // but it may still be relative to the working directory.
            fd = AT_FDCWD;
        }
        // unlinkat will disregard the file descriptor and call
        // rmdir/unlink on the path depending on the entity
        // type(file/directory).
        //
        // For deletion using the file descriptor, the path must be
        // relative to the directory the file descriptor points to.
        if (unlinkat(fd, dir_basename.c_str(), AT_REMOVEDIR) == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error removing directory \"" << dir_path << "\"");
        }
    };

    DirectoryTraversalFnPtr unlink_func = [](const char *file_path = nullptr,
                                             int fd = 0) {
        if (unlinkat(fd, file_path, 0) == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error removing file \"" << file_path << "\"");
        }
    };

    fileDescriptorTraverseAndApply(&root, rmdir_func, unlink_func,
                                   delete_root_directory, true);
}

void FileUtils::fileDescriptorTraverseAndApply(
    DirentWrapper *dir, DirectoryTraversalFnPtr dir_func,
    DirectoryTraversalFnPtr file_func, bool apply_to_root, bool pass_parent_fd)
{
    while (dir->entry() != nullptr) {
        if (dir->currentEntryIsDirectory()) {
            auto nextDir = dir->nextDir();
            fileDescriptorTraverseAndApply(&nextDir, dir_func, file_func, true,
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

std::string FileUtils::makePathAbsolute(const std::string &path,
                                        const std::string &cwd)
{
    if (cwd.empty() || cwd.front() != '/') {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "cwd must be an absolute path: ["
                                           << cwd << "]");
    }

    std::string full_path;
    if (path.front() != '/') {
        full_path = cwd + '/' + path;
    }
    else {
        full_path = path;
    }
    std::string normalized_path = FileUtils::normalizePath(full_path.c_str());

    // normalize_path removes trailing slashes, so let's preserve them here
    if (path.back() == '/' && normalized_path.back() != '/') {
        normalized_path.push_back('/');
    }
    return normalized_path;
}

std::string FileUtils::makePathRelative(const std::string &path,
                                        const std::string &cwd)
{
    // Return unmodified `path` in the following cases
    if (cwd.empty() || path.empty() || path.front() != '/') {
        return path;
    }

    // If `cwd` is set, require it to be an absolute path
    if (cwd.front() != '/') {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "cwd must be an absolute path or empty: cwd=[" << cwd << "]");
    }

    unsigned int pathIndex = 0;
    unsigned int nextPathIndex = pathIndex + 1;
    unsigned int lastMatchingSegmentEnd = 0;
    while (pathIndex < path.length() && path[pathIndex] == cwd[pathIndex]) {
        if (nextPathIndex == cwd.length()) {
            // Working directory is prefix of path, so if the last
            // segment matches, we're done.
            if (path.length() == nextPathIndex) {
                return path[pathIndex] == '/' ? "./" : ".";
            }
            else if (path.length() == pathIndex + 2 &&
                     path[nextPathIndex] == '/') {
                return "./";
            }
            else if (path[pathIndex] == '/') {
                return path.substr(nextPathIndex);
            }
            else if (path[nextPathIndex] == '/') {
                return path.substr(pathIndex + 2);
            }
        }
        else if (path[pathIndex] == '/') {
            lastMatchingSegmentEnd = pathIndex;
        }
        ++pathIndex;
        ++nextPathIndex;
    }

    if (pathIndex == path.length() && cwd[pathIndex] == '/') {
        // Path is prefix of working directory.
        if (nextPathIndex == cwd.length()) {
            return ".";
        }
        else {
            lastMatchingSegmentEnd = pathIndex;
            ++pathIndex;
            ++nextPathIndex;
        }
    }

    unsigned int dotdotsNeeded = 1;
    while (pathIndex < cwd.length()) {
        if (cwd[pathIndex] == '/' && cwd[nextPathIndex] != 0) {
            ++dotdotsNeeded;
        }
        ++pathIndex;
        ++nextPathIndex;
    }

    std::string result = std::string(path).replace(0, lastMatchingSegmentEnd,
                                                   dotdotsNeeded * 3 - 1, '.');

    for (unsigned int j = 0; j < dotdotsNeeded - 1; ++j) {
        result[j * 3 + 2] = '/';
    }
    return result;
}

std::string FileUtils::joinPathSegments(const std::string &firstSegment,
                                        const std::string &secondSegment,
                                        const bool forceSecondSegmentRelative)
{
    if (firstSegment.empty() || secondSegment.empty()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Both path segments must be non-empty."
                                           << " firstSegment=[" << firstSegment
                                           << "], secondSegment=["
                                           << secondSegment << "]");
    }

    const std::string firstSegmentNormalized =
        FileUtils::normalizePath(firstSegment.c_str());

    const std::string secondSegmentNormalized =
        FileUtils::normalizePath(secondSegment.c_str());

    if (!forceSecondSegmentRelative &&
        secondSegmentNormalized.front() == '/') {
        return secondSegmentNormalized;
    }
    else {
        // Now all we have to do is concatenate the paths with a '/' between
        // them and call normalizePath() to evaluate any remaining `..` and
        // make sure we remove any potential double '//' (e.g.
        // `joinPathSegments('a/','/b', true) -> '/a/b'`)
        const std::string combined_path =
            firstSegmentNormalized + '/' + secondSegmentNormalized;
        return FileUtils::normalizePath(combined_path.c_str());
    }
}

std::string
FileUtils::joinPathSegmentsNoEscape(const std::string &basedir,
                                    const std::string &pathWithinBasedir,
                                    const bool forceRelativePathWithinBaseDir)
{
    const std::string normalizedBaseDir =
        FileUtils::normalizePath(basedir.c_str());

    std::string normalizedPathWithinBaseDir =
        FileUtils::normalizePath(pathWithinBasedir.c_str());

    // Join paths using`joinPathSegments`
    const std::string joinedPath = FileUtils::joinPathSegments(
        basedir, normalizedPathWithinBaseDir, forceRelativePathWithinBaseDir);

    // Let's by default assume that the path escapes to reduce potential missed
    // cases in which we may incorrectly assume it doesn't escape, and check
    // for the known cases we are certain it doesn't escape
    bool escapes = true;

    // Do not allow any `..`, there shouldn't be any after normalizations
    // unless an escape is happening
    if (joinedPath.find("..") != std::string::npos) {
        escapes = true;
    }
    else if (joinedPath == normalizedBaseDir) {
        // Normalized path is the base directory
        escapes = false;
    }
    else if (normalizedBaseDir == "/" || normalizedBaseDir == "") {
        // Normalized baseDir is `/` or `` (root or relative to cwd)

        // In these cases not having `..` in the combinedPath is enough
        // to make sure it doesn't escape
        // (We checked for `..` above)
        escapes = false;
    }
    else if (joinedPath.find(normalizedBaseDir) == 0) {
        // Normalized path is within base directory

        // Make sure that this has a '/' as well after the `normalizedBaseDir`
        // Detects cases like:
        //       `joinPathSegmentsNoEscape('/base/dir', '../dir2')`
        // which would result in '/base/dir2', matching '/base/dir'
        // prefix but actually escaping `/base/dir`
        if (joinedPath.size() >=
            normalizedBaseDir.size()) { // sanity check, should always be the
                                        // case if we get here
            if (joinedPath.at(normalizedBaseDir.size()) == '/') {
                escapes = false;
            }
        }
    }

    // At this point the checks we made above should inform us whether this
    // joined path is escaping
    if (escapes) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Detected escaping path while joining "
                << "basedir=[" << basedir << "] and "
                << "pathWithinBasedir=[" << pathWithinBasedir << "]."
                << " Resulting escaping path=[" << joinedPath << "].");
    }
    return joinedPath;
}

std::string FileUtils::normalizePath(const char *path)
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
        else if (global && segment == ".." && segments.empty()) {
            // dot-dot in the root directory refers to the root directory
            // itself and can thus be dropped.
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
    else if (!global) {
        // The normalized path for the current directory is `.`,
        // not an empty string.
        result = ".";
    }
    return result;
}

std::string FileUtils::pathBasename(const char *path)
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
