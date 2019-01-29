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

#include <string>

namespace buildboxcommon {

#define DEFAULT_TMP_PREFIX "buildbox"

class TemporaryFile {
  public:
    /**
     * Create a temporary file on disk. If a prefix is specified, it
     * will be included in the name of the temporary file.
     */
    TemporaryFile(const char *prefix = DEFAULT_TMP_PREFIX);

    /**
     * Delete the temporary file.
     */
    ~TemporaryFile();

    const char *name() const { return d_name.c_str(); };

    const int fd() const { return d_fd; };

    void close();

  private:
    std::string d_name;
    int d_fd;
};

class TemporaryDirectory {
  public:
    /**
     * Create a temporary directory on disk. If a prefix is specified, it
     * will be included in the name of the temporary directory.
     */
    TemporaryDirectory(const char *prefix = DEFAULT_TMP_PREFIX);

    /**
     * Delete the temporary directory.
     */
    ~TemporaryDirectory();

    const char *name() const { return d_name.c_str(); };

  private:
    std::string d_name;
};

/**
 * Create a directory if it doesn't already exist, including parents.
 */
void create_directory(const char *path);

/**
 * Delete an existing directory.
 */
void delete_directory(const char *path);

/**
 * Return true if the given file path is executable.
 */
bool is_executable(const char *path);

/**
 * Make the given file executable.
 */
void make_executable(const char *path);

} // namespace buildboxcommon

#endif
