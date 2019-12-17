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

#include <buildboxcommon_temporaryfile.h>
#include <dirent.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

const char *TemporaryFileDefaults::DEFAULT_TMP_DIR =
    TempDefaults::DEFAULT_TMP_DIR;

TemporaryFile::TemporaryFile(const char *prefix)
{
    create(tempDirectory(), prefix);
}

TemporaryFile::TemporaryFile(const char *directory, const char *prefix,
                             mode_t mode)
{
    create(directory, prefix, mode);
}

TemporaryFile::~TemporaryFile()
{
    if (this->d_fd != -1) {
        ::close(this->d_fd);
    }
    unlink(this->d_name.c_str());
}

void TemporaryFile::close()
{
    ::close(this->d_fd);
    this->d_fd = -1;
}

const char *TemporaryFile::tempDirectory()
{
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir == nullptr || tmpdir[0] == '\0') {
        return TemporaryFileDefaults::DEFAULT_TMP_DIR;
    }
    return tmpdir;
}

void TemporaryFile::create(const char *directory, const char *prefix,
                           mode_t mode)
{
    std::string name =
        std::string(directory) + "/" + std::string(prefix) + "XXXXXX";

    this->d_fd = mkstemp(&name[0]);
    if (this->d_fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    /* mkstemp creates files with mode 0600 */
    if (mode != 0600) {
        fchmod(this->d_fd, mode);
    }

    name = FileUtils::normalize_path(name.c_str());
    this->d_name = std::move(name);
}

} // namespace buildboxcommon
