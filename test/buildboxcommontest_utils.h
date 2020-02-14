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

#ifndef INCLUDED_BUILDBOXCOMMONTEST_UTILS
#define INCLUDED_BUILDBOXCOMMONTEST_UTILS

#include <buildboxcommon_fileutils.h>
#include <dirent.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>

using namespace buildboxcommon;

namespace buildboxcommontest {

struct TestUtils {
    static bool pathExists(const char *path)
    {
        struct stat statResult;
        return stat(path, &statResult) == 0;
    }

    static void touchFile(const char *path)
    {
        std::fstream fs;
        fs.open(path, std::ios::out);
        fs.close();
    }

    static std::string createSubDirectory(const std::string &root_path,
                                          const std::string &subdir_name)
    {
        auto subdir = root_path + "/" + subdir_name;
        FileUtils::createDirectory(subdir.c_str());
        return subdir;
    }

    static std::string createFileInDirectory(const std::string &file_name,
                                             const std::string &dir_name)
    {
        const auto file_in_dir = dir_name + "/" + file_name;
        touchFile(file_in_dir.c_str());
        assert(pathExists(file_in_dir.c_str()));
        return file_in_dir;
    }
};

} // namespace buildboxcommontest

#endif
