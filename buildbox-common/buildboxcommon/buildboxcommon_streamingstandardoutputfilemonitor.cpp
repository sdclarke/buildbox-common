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

namespace {
size_t readBufferSizeBytes()
{
    const auto pageSizeBytes = sysconf(_SC_PAGESIZE);
    if (pageSizeBytes == -1) {
        const auto defaultBufferSizeBytes = 4096;
        BUILDBOX_LOG_ERROR("Could not read `sysconf(_SC_PAGESIZE)`, setting "
                           "the size of the read buffer to "
                           << defaultBufferSizeBytes << "bytes");
        return defaultBufferSizeBytes;
    }

    BUILDBOX_LOG_TRACE("Setting the size of the read buffer to "
                       << pageSizeBytes << " bytes");
    return static_cast<size_t>(pageSizeBytes);
}

} // namespace

namespace buildboxcommon {

const std::chrono::milliseconds
    StreamingStandardOutputFileMonitor::s_pollInterval(500);

StreamingStandardOutputFileMonitor::StreamingStandardOutputFileMonitor(
    const std::string &path, const DataReadyCallback &readCallback)
    : d_filePath(path), d_fileFd(openFile(d_filePath)),
      d_dataReadyCallback(readCallback), d_stopRequested(false),
      d_monitoringThread(&StreamingStandardOutputFileMonitor::monitorFile,
                         this)
{
}

StreamingStandardOutputFileMonitor::~StreamingStandardOutputFileMonitor()
{
    stop();
    close(d_fileFd);
}

void StreamingStandardOutputFileMonitor::stop()
{
    if (!d_stopRequested) {
        d_stopRequested = true;
        d_monitoringThread.join();
    }
}

int StreamingStandardOutputFileMonitor::openFile(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error opening file " << path);
    }
    return fd;
}

bool StreamingStandardOutputFileMonitor::fileHasData() const
{
    struct stat st;
    if (fstat(d_fileFd, &st) == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error calling fstat()");
    }

    return (st.st_size > 0);
}

void StreamingStandardOutputFileMonitor::monitorFile() const
{
    const int fd = openFile(d_filePath);
    BUILDBOX_LOG_TRACE("Started monitoring thread for " << d_filePath);

    // Polling the file until it has data available for reading or being
    // asked to stop:
    while (!fileHasData()) {
        std::this_thread::sleep_for(s_pollInterval);

        if (d_stopRequested) {
            BUILDBOX_LOG_TRACE("Stop requested. File "
                               << d_filePath << " was never written.");
            return;
        }
    }

    std::unique_ptr<char[]> buffer(new char[readBufferSizeBytes()]);
    BUILDBOX_LOG_TRACE("Data available from " << d_filePath
                                              << ". Starting to read.");
    while (true) {
        const ssize_t bytesRead = read(fd, buffer.get(), sizeof(buffer));
        if (bytesRead == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error reading file " << d_filePath);
        }

        if (bytesRead > 0) {
            BUILDBOX_LOG_TRACE("Read " << bytesRead << " bytes from "
                                       << d_filePath);
            d_dataReadyCallback(
                FileChunk(buffer.get(), static_cast<size_t>(bytesRead)));
        }
        else if (d_stopRequested) {
            return;
        }
        else {
            std::this_thread::sleep_for(s_pollInterval);
        }
    }
}

} // namespace buildboxcommon
