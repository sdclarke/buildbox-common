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

#include <buildboxcommon_streamingstandardoutputstatfilemonitor.h>

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

// Invoke `dataReadyCallback()` with at least this number of bytes:
const size_t
    StreamingStandardOutputStatFileMonitor::s_min_write_batch_size_bytes = 100;

const std::chrono::milliseconds
    StreamingStandardOutputStatFileMonitor::s_pollInterval(10);

StreamingStandardOutputStatFileMonitor::StreamingStandardOutputStatFileMonitor(
    const std::string &path, const DataReadyCallback &readCallback)
    : d_filePath(path), d_fileFd(openFile(d_filePath)),
      d_dataReadyCallback(readCallback), d_stopRequested(false),
      d_monitoringThread(&StreamingStandardOutputStatFileMonitor::monitorFile,
                         this),
      d_read_buffer_size(readBufferSizeBytes()),
      d_read_buffer(std::make_unique<char[]>(d_read_buffer_size)),
      d_read_buffer_offset(0)
{
}

StreamingStandardOutputStatFileMonitor::
    ~StreamingStandardOutputStatFileMonitor()
{
    stop();
    close(d_fileFd);
}

void StreamingStandardOutputStatFileMonitor::stop()
{
    if (!d_stopRequested) {
        d_stopRequested = true;
        d_monitoringThread.join();
    }
}

int StreamingStandardOutputStatFileMonitor::openFile(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "Error opening file " << path);
    }
    return fd;
}

bool StreamingStandardOutputStatFileMonitor::waitForInitialFileWrite() const
{
    struct stat st;

    while (!d_stopRequested) {
        if (fstat(d_fileFd, &st) == -1) {
            const auto errorReason = strerror(errno);
            BUILDBOX_LOG_ERROR("Error calling fstat() for ["
                               << d_filePath << "]: " << errorReason);
            return false;
        }

        if (st.st_size > 0) {
            return true;
        }

        std::this_thread::sleep_for(s_pollInterval);
    }

    BUILDBOX_LOG_TRACE("Stop requested. File " << d_filePath
                                               << " was never written.");
    return false;
}

void StreamingStandardOutputStatFileMonitor::monitorFile()
{
    BUILDBOX_LOG_TRACE("Started monitoring thread for " << d_filePath);

    // Polling the file until it has data available for reading or being
    // asked to stop:
    const bool fileHasData = waitForInitialFileWrite();
    if (!fileHasData) {
        return;
    }

    BUILDBOX_LOG_TRACE("Data available from " << d_filePath
                                              << ". Starting to read.");
    while (true) {
        const auto bufferFreeBytes =
            d_read_buffer_size - d_read_buffer_offset - 1;

        const auto bufferWriteStart =
            d_read_buffer.get() + d_read_buffer_offset;
        const ssize_t bytesRead =
            read(d_fileFd, bufferWriteStart, bufferFreeBytes);
        if (bytesRead == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Error reading file " << d_filePath);
        }

        BUILDBOX_LOG_TRACE("Read " << bytesRead << " bytes from "
                                   << d_filePath);

        d_read_buffer_offset += static_cast<size_t>(bytesRead);

        // If we have enough data or it's the last dump, invoke the callback
        // function to empty the buffer:
        const bool batch_larger_than_min =
            d_read_buffer_offset > s_min_write_batch_size_bytes;
        const bool last_write = d_read_buffer_offset > 0 && d_stopRequested;
        if (batch_larger_than_min || last_write) {
            d_dataReadyCallback(
                FileChunk(d_read_buffer.get(), d_read_buffer_offset));
            d_read_buffer_offset = 0;

            if (last_write) {
                return;
            }
        }
        else if (bytesRead == 0 && d_stopRequested) {
            return;
        }
        else {
            std::this_thread::sleep_for(s_pollInterval);
        }
    }
}

} // namespace buildboxcommon
