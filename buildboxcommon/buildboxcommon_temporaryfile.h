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

#ifndef INCLUDED_BUILDBOXCOMMON_TEMPORARYFILE
#define INCLUDED_BUILDBOXCOMMON_TEMPORARYFILE

#include <buildboxcommon_fileutils.h>

namespace buildboxcommon {

class TemporaryFile {
    // Provide a TemporaryFile component that provides the facility for
    // temporary files that are scoped in an RAII fashion.
  public:
    /**
     * Create a temporary file on disk. If a prefix is specified, it
     * will be included in the name of the temporary file.
     */
    TemporaryFile(const char *prefix = FileUtilsDefaults::DEFAULT_TMP_PREFIX);

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
} // namespace buildboxcommon
#endif
