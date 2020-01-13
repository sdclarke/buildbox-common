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

#include <buildboxcommon_temporarydirectory.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

namespace buildboxcommon {

namespace {

const char *constructTmpDirPath(const char *path)
{
    if (path == nullptr) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "NULL path was passed into TemporaryDirectory");
    }
    if (strlen(path) == 0) {
        const char *tmpdir = getenv("TMPDIR");
        if (tmpdir == nullptr || tmpdir[0] == '\0') {
            tmpdir = TempDefaults::DEFAULT_TMP_DIR;
        }

        return tmpdir;
    }
    else {
        return path;
    }
}

} // unnamed namespace

TemporaryDirectory::TemporaryDirectory(const char *prefix)
    : TemporaryDirectory("", prefix)
{
}

TemporaryDirectory::TemporaryDirectory(const char *path, const char *prefix)
    : d_name(create(constructTmpDirPath(path), prefix))
{
}

std::string TemporaryDirectory::create(const char *path, const char *prefix)
{
    std::string name =
        std::string(path) + "/" + std::string(prefix) + "XXXXXX";

    // Normalize the path.
    name = FileUtils::normalize_path(name.c_str());

    if (mkdtemp(&name[0]) == nullptr) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error in mkdtemp");
    }

    return name;
}

void TemporaryDirectory::setAutoRemove(bool auto_remove)
{
    d_auto_remove = auto_remove;
}

TemporaryDirectory::~TemporaryDirectory()
{
    if (d_auto_remove) {
        FileUtils::delete_directory(this->d_name.c_str());
    }
}

} // namespace buildboxcommon
