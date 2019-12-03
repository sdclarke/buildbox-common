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

#include <buildboxcommon_direntwrapper.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace buildboxcommon;

DirentWrapper::DirentWrapper(const std::string &path)
    : d_dir(nullptr), d_entry(nullptr), d_path(path), d_fd(-1), d_p_fd(-1)
{
    openDir();
}

DirentWrapper::DirentWrapper(const int fd, const int p_fd,
                             const std::string &path)
    : d_entry(nullptr), d_path(path), d_fd(fd), d_p_fd(p_fd)
{
    d_dir = fdopendir(fd);
    if (d_dir == nullptr) {
        int errsv = errno;
        const std::string error_message =
            "Error opening directory from file descriptor at path: [" +
            d_path + "]: " + strerror(errsv);
        close(fd);
        BUILDBOX_LOG_ERROR(error_message);
        throw std::runtime_error(error_message);
    }
    next();
}

DirentWrapper::DirentWrapper(DIR *dir, const std::string &path)
    : d_dir(dir), d_entry(nullptr), d_path(path), d_fd(-1)
{
    openDir();
}

bool DirentWrapper::currentEntryIsFile() const
{
    struct stat statResult;
    bool ret_val = false;
    if (d_entry == nullptr) {
        return ret_val;
    }
    if (fstatat(d_fd, d_entry->d_name, &statResult, AT_SYMLINK_NOFOLLOW) ==
        0) {
        ret_val = S_ISREG(statResult.st_mode);
    }
    else {
        const std::string error_message =
            "Unable to stat entity: [" + std::string(d_entry->d_name) + "]";
        BUILDBOX_LOG_ERROR(error_message);
        throw std::runtime_error(error_message);
    }
    return ret_val;
}

bool DirentWrapper::currentEntryIsDirectory() const
{
    struct stat statResult;
    bool ret_val = false;
    if (d_entry == nullptr) {
        return ret_val;
    }
    if (fstatat(d_fd, d_entry->d_name, &statResult, AT_SYMLINK_NOFOLLOW) ==
        0) {
        ret_val = S_ISDIR(statResult.st_mode);
    }
    else {
        const std::string error_message =
            "Unable to stat entity: [" + std::string(d_entry->d_name) + "]";
        BUILDBOX_LOG_ERROR(error_message);
        throw std::runtime_error(error_message);
    }
    return ret_val;
}

DirentWrapper DirentWrapper::nextDir() const
{
    int next_fd = this->openEntry(O_DIRECTORY);
    if (next_fd == -1) {
        throw std::runtime_error("Error getting dir from non-directory.");
    }

    return DirentWrapper(next_fd, this->fd(), this->currentEntryPath());
}

const dirent *DirentWrapper::entry() const { return d_entry; }

int DirentWrapper::fd() const { return d_fd; }

int DirentWrapper::pfd() const { return d_p_fd; }

int DirentWrapper::openEntry(int flag) const
{
    if (d_entry == nullptr) {
        return -1;
    }

    int fd = openat(dirfd(d_dir), d_entry->d_name, flag);
    if (fd == -1) {
        const std::string error_message =
            "Warning: when trying to open file descriptor representing path "
            "with "
            "openat: [" +
            this->d_path + "/" + d_entry->d_name + "] " + strerror(errno);
        BUILDBOX_LOG_WARNING(error_message);
    }
    return fd;
}

const std::string DirentWrapper::path() const { return d_path; }

const std::string DirentWrapper::currentEntryPath() const
{
    if (d_entry == nullptr) {
        return "";
    }
    else {
        const std::string current_path =
            d_path + "/" + std::string(d_entry->d_name);
        return current_path;
    }
}

void DirentWrapper::operator++() { next(); }

DirentWrapper::~DirentWrapper()
{
    if (d_dir != nullptr) {
        // Close the directory entry;
        const int ret_val = closedir(d_dir);
        if (ret_val != 0) {
            const int errsv = errno;
            const std::string error_message = "Error closing directory: [" +
                                              d_path + "]: " + strerror(errsv);
            BUILDBOX_LOG_WARNING(error_message);
        }
    }
}

void DirentWrapper::openDir()
{
    // Get DIR pointer.
    if (d_dir == nullptr) {
        d_dir = opendir(d_path.c_str());
        if (d_dir == nullptr) {
            const int errsv = errno;
            const std::string error_message = "Error opening directory: [" +
                                              d_path + "]: " + strerror(errsv);
            BUILDBOX_LOG_ERROR(error_message);
            throw std::runtime_error(error_message);
        }
    }
    // Get directory file descriptor.
    if (d_fd < 0) {
        d_fd = dirfd(d_dir);
        if (d_fd < 0) {
            const int errsv = errno;
            const std::string error_message =
                "Error opening directory file descriptor at path: [" + d_path +
                "]: " + strerror(errsv);
            BUILDBOX_LOG_ERROR(error_message);
            throw std::runtime_error(error_message);
        }
        this->next();
    }
}

void DirentWrapper::next()
{
    // Skip "." and ".." if d_entry is defined.
    do {
        errno = 0;
        d_entry = readdir(d_dir);
        if (errno != 0 && d_entry == nullptr) {
            int errsv = errno;
            const std::string error_message =
                "Error reading from directory: [" + d_path +
                "]: " + strerror(errsv);
            BUILDBOX_LOG_ERROR(error_message);
            throw std::runtime_error(error_message);
        }
    } while (d_entry != nullptr && (strcmp(d_entry->d_name, ".") == 0 ||
                                    strcmp(d_entry->d_name, "..") == 0));
}
