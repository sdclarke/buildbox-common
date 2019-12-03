/*
 * Copyright 2020 Bloomberg Finance LP
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

#ifndef INCLUDED_BUILDBOXCOMMON_DIRENTWRAPPER
#define INCLUDED_BUILDBOXCOMMON_DIRENTWRAPPER

#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace buildboxcommon {

/*
 * This class implements a RAII wrapper around the dirent library allowing for
 * easy, efficient and thread safe traversal of a DIR struct, using file
 * descriptors.
 *
 * Example usage:
 *
 * std::string file_path = "/etc/bin";
 * DirentWrapper dir(file_path);
 *
 * while(dir.entry() != nullptr)
 * {
 *    ....
 *    dir.next();
 * }
 *
 */
class DirentWrapper final {
  public:
    explicit DirentWrapper(const std::string &path);
    explicit DirentWrapper(DIR *dir, const std::string &path);

    /*
     * Return true if stat of the current entry ISREG.
     * If nullptr, return false.
     */
    bool currentEntryIsFile() const;

    /*
     * Return true if stat of the current entry ISDIR.
     * If nullptr, return false.
     */
    bool currentEntryIsDirectory() const;

    /*
     * Return a DirentWrapper of the current d_entry, if a directory.
     * Else, throw.
     */
    DirentWrapper nextDir() const;

    /*
     * Read the next entry, set d_entry.
     */
    void next();

    /*
     * Return a pointer to the current entry.
     */
    const dirent *entry() const;

    /*
     * Return the current directories file descriptor.
     */
    int fd() const;

    /*
     * Return the parent directories file descriptor.
     */
    int pfd() const;

    /*
     * Return the a file descriptor that points the current entry.
     * The flag is passed in to openat. Find all available flags in fcntl.h
     *
     * If there is an error opening the entry:  return -1.
     */
    int openEntry(int flag) const;

    /*
     * Return the full path of the current directory iterating over.
     */
    const std::string path() const;

    /*
     * Return the full path of the current entity.
     */
    const std::string currentEntryPath() const;
    void operator++();
    ~DirentWrapper();

  private:
    explicit DirentWrapper(const int fd, const int p_fd,
                           const std::string &path);
    /*
     * Open the directory stream, using d_path store in d_dir.
     * If d_fd is negative, call dirfd on the directory stream to get it's file
     * discriptor.
     */
    void openDir();

    /*
     * The current directory stream iterating over. closedir is called on this
     * stream on destruction.
     */
    DIR *d_dir;

    /*
     * The current dirent in d_dir.
     */
    dirent *d_entry;

    /*
     * The current path of the directory.
     */
    std::string d_path;

    /*
     * The file descriptor representing the directory at d_path.
     */
    int d_fd;

    /*
     * The file descriptor representing the parent directory of this DIR.
     *
     * Set if the class is constructed from a previous DirentWrapper.
     * Otherwise, -1.
     */
    int d_p_fd;
};

} // namespace buildboxcommon
#endif
