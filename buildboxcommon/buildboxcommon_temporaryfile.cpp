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
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

TemporaryFile::TemporaryFile(const char *prefix)
{
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir == nullptr || tmpdir[0] == '\0') {
        tmpdir = "/tmp";
    }
    std::string name =
        std::string(tmpdir) + "/" + std::string(prefix) + "XXXXXX";
    this->d_fd = mkstemp(&name[0]);
    if (this->d_fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    this->d_name = name;
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

} // namespace buildboxcommon
