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

#ifndef INCLUDED_BUILDBOXCOMMON_FILEUTILS
#define INCLUDED_BUILDBOXCOMMON_FILEUTILS

#include <buildboxcommon_direntwrapper.h>
#include <buildboxcommon_tempconstants.h>

#include <chrono>
#include <dirent.h>
#include <functional>
#include <google/protobuf/util/time_util.h>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace buildboxcommon {

struct FileUtils {
    // Provide a namespace for file utilities.

    /**
     * Return true if the given path represents a directory.
     */
    static bool isDirectory(const char *path);
    static bool isDirectory(int fd);
    /**
     * Do not follow symlinks
     */
    static bool isDirectoryNoFollow(const char *path);

    /**
     * Return true if the given path represents a regular file.
     */
    static bool isRegularFile(const char *path);
    /**
     * Do not follow symlinks
     */
    static bool isRegularFileNoFollow(const char *path);

    /**
     * Return true if the given path represents a symlink.
     */
    static bool isSymlink(const char *path);

    /**
     * Return true if the directory is empty.
     */
    static bool directoryIsEmpty(const char *path);

    /**
     * Create a directory if it doesn't already exist, including parents.
     * Create the directory with specified mode, by default 0777 -rwxrwxrwx.
     */
    static void createDirectory(const char *path, mode_t mode = 0777);

    /**
     * Delete an existing directory.
     */
    static void deleteDirectory(const char *path);

    /**
     * Delete the contents of an existing directory.
     */
    static void clearDirectory(const char *path);

    /**
     * Return true if the given file path is executable.
     */
    static bool isExecutable(const char *path);

    /**
     * Return true if a given file descriptor is executable.
     */
    static bool isExecutable(int fd);

    /**
     * Return the size in bytes of the file specified by the given path.
     */
    static int64_t getFileSize(const char *path);

    /**
     * Return the size in bytes of the file specified by the given file
     * descriptor.
     */
    static int64_t getFileSize(int fd);

    /**
     * Return a time point in seconds representing the mtime of the file
     * specified by the given path.
     */
    static std::chrono::system_clock::time_point
    getFileMtime(const char *path);

    /**
     * Return a time point in seconds representing the mtime of the file
     * specified by the given file descriptor.
     */
    static std::chrono::system_clock::time_point getFileMtime(const int fd);

    /**
     * Modify the mtime of an existing file to the time represented by the
     * given time_point. The file is described by the given file
     * descriptor.
     */
    static void setFileMtime(const int fd,
                             std::chrono::system_clock::time_point timepoint);

    /**
     * Modify the mtime of an existing file to the time represented by the
     * given time_point. The file is described by the given path.
     */
    static void setFileMtime(const char *path,
                             std::chrono::system_clock::time_point timepoint);

    /**
     * Make the given file executable.
     */
    static void makeExecutable(const char *path);

    /**
     * Gets the contents of a file
     */
    static std::string getFileContents(const char *path);

    /**
     * Simplify the given path.
     *
     * The returned path will not contain any empty or `.` segments, and any
     * `..` segments will occur at the start of the path.
     */
    static std::string normalizePath(const char *path);

    /**
     * Return the basename of the given path.
     *
     * The returned entity will be last segment of the path.
     * If no segments found, will an empty string.
     */
    static std::string pathBasename(const char *path);

    /**
     * Make the given path absolute, using the current working directory.
     *
     * `cwd` must be an absolute path, otherwise it throws an
     * `std::runtime_error` exception.
     */
    static std::string makePathAbsolute(const std::string &path,
                                        const std::string &cwd);

    /**
     * Make the given path relative to the given working directory.
     *
     * If the given working directory is empty, or if the given path has
     * nothing to do with the working directory, the path will be returned
     * unmodified.
     */
    static std::string makePathRelative(const std::string &path,
                                        const std::string &cwd);

    /**
     * Join two path segments together and return the normalized results.
     * - When the second segment is an absolute path, it will be the only
     * path included in the (normalized) result, similar to other
     * implementations of standard libraries that join paths,
     * unless the `forceSecondSegmentRelative` flag is set to true (Default:
     * false)
     * - Warning: When the paths include `..`, the resulting joined path may
     * escape the first path
     *
     * e.g. `joinPathSegments('/a/', '/b')`       -> '/b'
     *      `joinPathSegments('/a/', '/b', true)` -> '/a/b'
     */
    static std::string
    joinPathSegments(const std::string &firstSegment,
                     const std::string &secondSegment,
                     const bool forceSecondSegmentRelative = false);

    /**
     * Join two path segments together (using FileUtils::joinPathSegments), but
     * throw a `std::runtime_error` if the second path segment makes the joined
     * path escape the first path segment.
     *
     * Accepts the optional flag `forceRelativePathWithinBaseDir` for invoking
     * `joinPathSegments` (Default: false)
     *
     * e.g. `joinPathSegmentsNoEscape('/a/', '/b')`       -> raises
     * `std::runtime_error` due to `/b` escaping '/a/'
     *      `joinPathSegmentsNoEscape('/a/', '/b', true)` -> '/a/b'
     */
    static std::string joinPathSegmentsNoEscape(
        const std::string &basedir, const std::string &pathWithinBasedir,
        const bool forceRelativePathWithinBaseDir = false);

    /**
     * Copy file contents (non-atomically) from the given source path
     * to the given destination path. Additionally attempt to
     * duplicate the file mode.
     */
    static void copyFile(const char *src_path, const char *dest_path);

    /**
     * Write a file atomically. Note that this only guarantees thread safety
     * if the actual contents to be written to `path` are the same across
     * threads. If the file already exists, it will be replaced atomically.
     *
     * (To guarantee atomicity, create a temporary file, write the data to it
     * and rename the temporary file to the final path.)
     *
     * `mode` allows setting the permissions for the created file; by default
     * 0600 (the default used by `mkstemp()`).
     *
     * If `intermediate_directory` is specified, the temporary file is created
     * in that location. It must be contained in the same filesystem than the
     * output `path` in order for `rename(2)` to work.
     *
     * On errors during writing the data or renaming, throw an
     * `std::system_error` exception.
     */
    static void writeFileAtomically(
        const std::string &path, const std::string &data, mode_t mode = 0600,
        const std::string &intermediate_directory = "",
        const std::string &prefix = TempDefaults::DEFAULT_TMP_PREFIX);

    /**
     * Traverse and apply functions on files and directories recursively.
     *
     * If apply_to_root is true, dir_func is applied to the directory stream
     * the function is initally called with.
     *
     * If pass_parent_fd is true, the parent directory of dir will be passed
     * into dir_func instead of dir. This is useful in the case of deletion.
     */
    typedef std::function<void(const char *path, int fd)>
        DirectoryTraversalFnPtr;

    static void fileDescriptorTraverseAndApply(
        DirentWrapper *dir, DirectoryTraversalFnPtr dir_func = nullptr,
        DirectoryTraversalFnPtr file_func = nullptr,
        bool apply_to_root = false, bool pass_parent_fd = false);

  private:
    /**
     * Return the stat of the file at the given open file descriptor.
     */
    static struct stat getFileStat(const int fd);

    /**
     * Return the stat of the file at the given path.
     */
    static struct stat getFileStat(const char *path);

    /**
     * Return a time point in seconds representing the st_mtim of the filestat.
     */
    static std::chrono::system_clock::time_point
    getMtimeTimepoint(struct stat &result);

    /**
     * Deletes the contents of an existing directory.
     * `delete_parent_directory` allows specifying whether the top-level
     * directory in `path` is to be deleted.
     */
    static void deleteRecursively(const char *path,
                                  const bool delete_parent_directory);

    /**
     * Given a path, create the directory including its parents if necessary.
     * Silently ignore existing directories.
     * PRE: `path` must be normalized.
     */
    static void createDirectoriesInPath(const std::string &path,
                                        const mode_t mode);
};
} // namespace buildboxcommon

#endif
