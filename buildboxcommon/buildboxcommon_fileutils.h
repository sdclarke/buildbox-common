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
    static bool is_directory(const char *path);

    static bool is_directory(const int fd);

    /**
     * Return true if the given path represents a regular file.
     */
    static bool is_regular_file(const char *path);

    /**
     * Return true if the given path represents a symlink.
     */
    static bool is_symlink(const char *path);

    /**
     * Return true if the directory is empty.
     */
    static bool directory_is_empty(const char *path);

    /**
     * Create a directory if it doesn't already exist, including parents.
     * Create the directory with specified mode, by default 0777 -rwxrwxrwx.
     */
    static void create_directory(const char *path, mode_t mode = 0777);

    /**
     * Delete an existing directory.
     */
    static void delete_directory(const char *path);

    /**
     * Delete the contents of an existing directory.
     */
    static void clear_directory(const char *path);

    /**
     * Return true if the given file path is executable.
     */
    static bool is_executable(const char *path);

    /**
     * Return true if a given file descriptor is executable.
     */
    static bool is_executable(const int fd);

    /**
     * Return a time point in seconds representing the mtime of the file
     * specified by the given path.
     */
    static std::chrono::system_clock::time_point
    get_file_mtime(const char *path);

    /**
     * Return a time point in seconds representing the mtime of the file
     * specified by the given file descriptor.
     */
    static std::chrono::system_clock::time_point get_file_mtime(const int fd);

    /**
     * Modify the mtime of an existing file to the time represented by the
     * given time_point. The file is described by the given file
     * descriptor.
     */
    static void
    set_file_mtime(const int fd,
                   std::chrono::system_clock::time_point timepoint);

    /**
     * Modify the mtime of an existing file to the time represented by the
     * given time_point. The file is described by the given path.
     */
    static void
    set_file_mtime(const char *path,
                   std::chrono::system_clock::time_point timepoint);

    /**
     * Make the given file executable.
     */
    static void make_executable(const char *path);

    /**
     * Gets the contents of a file
     */
    static std::string get_file_contents(const char *path);

    /**
     * Simplify the given path.
     *
     * The returned path will not contain any empty or `.` segments, and any
     * `..` segments will occur at the start of the path.
     */
    static std::string normalize_path(const char *path);

    /**
     * Return the basename of the given path.
     *
     * The returned entity will be last segment of the path.
     * If no segments found, will an empty string.
     */
    static std::string path_basename(const char *path);

    /**
     * Make the given path absolute, using the current working directory.
     *
     * `cwd` must be an absolute path, otherwise it throws an
     * `std::runtime_error` exception.
     */
    static std::string make_path_absolute(const std::string &path,
                                          const std::string &cwd);

    /**
     * Copy file contents (non-atomically) from the given source path
     * to the given destination path. Additionally attempt to
     * duplicate the file mode.
     */
    static void copy_file(const char *src_path, const char *dest_path);

    /**
     * Write a file atomically. Note that this only guarantees thread safety
     * if the actual contents to be written to `path` are the same across
     * threads.
     *
     * (To guarantee atomicity, create a temporary file, write the data to it,
     * create a hard link from the destination to the temporary file, and
     * `unlink()` the temporary file.)
     *
     * `mode` allows setting the permissions for the created file; by default
     * 0600 (the default used by `mkstemp()`).
     *
     * If `intermediate_directory` is specified, the temporary file is created
     * in that location. It must be contained in the same filesystem than the
     * output `path` in order for hard links to work.
     *
     * Return 0 if successful or the `errno` value as set by `link(2)` if not.
     *
     * On errors during writing the data, throw an `std::system_error`
     * exception.
     */
    static int write_file_atomically(
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

    static void FileDescriptorTraverseAndApply(
        DirentWrapper *dir, DirectoryTraversalFnPtr dir_func = nullptr,
        DirectoryTraversalFnPtr file_func = nullptr,
        bool apply_to_root = false, bool pass_parent_fd = false);

  private:
    /**
     * Return the stat of the file at the given open file descriptor.
     */
    static struct stat get_file_stat(const int fd);

    /**
     * Return the stat of the file at the given path.
     */
    static struct stat get_file_stat(const char *path);

    /**
     * Return a time point in seconds representing the st_mtim of the filestat.
     */
    static std::chrono::system_clock::time_point
    get_mtime_timepoint(struct stat &result);

    /**
     * Deletes the contents of an existing directory.
     * `delete_parent_directory` allows specifying whether the top-level
     * directory in `path` is to be deleted.
     */
    static void delete_recursively(const char *path,
                                   const bool delete_parent_directory);
};
} // namespace buildboxcommon

#endif
