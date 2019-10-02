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

#include <buildboxcommon_tempconstants.h>

#include <string>

namespace buildboxcommon {

struct FileUtils {
    // Provide a namespace for file utilities.

    /**
     * Return true if the given file path is a directory.
     */
    static bool is_directory(const char *path);

    /**
     * Return true if the given file path is a regular file.
     */
    static bool is_regular_file(const char *path);

    /**
     * Return true if the directory is empty.
     */
    static bool directory_is_empty(const char *path);

    /**
     * Create a directory if it doesn't already exist, including parents.
     */
    static void create_directory(const char *path);

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
     * Make the given path absolute, using the current working directory.
     *
     * `cwd` must be an absolute path, otherwise it throws an
     * `std::runtime_error` exception.
     */
    static std::string make_path_absolute(const std::string &path,
                                          const std::string &cwd);
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

  private:
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
