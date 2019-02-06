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

#ifndef INCLUDED_BUILDBOXCOMMON_TEMPORARYDIRECTORY
#define INCLUDED_BUILDBOXCOMMON_TEMPORARYDIRECTORY

#include <buildboxcommon_fileutils.h>
#include <string>

namespace buildboxcommon {
class TemporaryDirectory {
  public:
    /**
     * Create a temporary directory on disk. If a prefix is specified, it
     * will be included in the name of the temporary directory.
     */
    explicit TemporaryDirectory(
        const char *prefix = FileUtilsDefaults::DEFAULT_TMP_PREFIX);

    /**
     * Delete the temporary directory.
     */
    ~TemporaryDirectory();

    const char *name() const { return d_name.c_str(); };

  private:
    std::string d_name;
};

} // namespace buildboxcommon
#endif
