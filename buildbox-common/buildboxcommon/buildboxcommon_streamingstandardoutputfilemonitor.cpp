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

#include <buildboxcommon_streamingstandardoutputfilemonitor.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_logging.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int buildboxcommon::StreamingStandardOutputFileMonitor::openFile(
    const std::string &path)
{

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error opening file " << path);
    }
    return fd;
}

size_t
buildboxcommon::StreamingStandardOutputFileMonitor::readBufferSizeBytes()
{
    const auto pageSizeBytes = sysconf(_SC_PAGESIZE);
    if (pageSizeBytes == -1) {
        const auto defaultBufferSizeBytes = 4096;
        BUILDBOX_LOG_WARNING("Could not read `sysconf(_SC_PAGESIZE)`, setting "
                             "the size of the read buffer to "
                             << defaultBufferSizeBytes << "bytes");
        return defaultBufferSizeBytes;
    }

    BUILDBOX_LOG_TRACE("Setting the size of the read buffer to "
                       << pageSizeBytes << " bytes");
    return static_cast<size_t>(pageSizeBytes);
}
