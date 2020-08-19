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

#include <buildboxcommon_streamingstandardoutputinotifyfilemonitor.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_logging.h>

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// The `inotify_event` entry has a variable-length `name` field at the end.
// That name contains a path, so we consider the longest possible path for an
// upper bound.
#define INOTIFY_EVENT_MAX_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)

// We'll be monitoring `IN_MODIFY` and `IN_CLOSE_WRITE` events, if they are
// available simultaneously we'll fetch them with a single `read()`.
#define INOTIFY_MAX_NUMBER_OF_EVENTS 2
#define INOTIFY_BUFFER_SIZE                                                   \
    ((INOTIFY_MAX_NUMBER_OF_EVENTS) * (INOTIFY_EVENT_MAX_SIZE))

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

StreamingStandardOutputInotifyFileMonitor::
    StreamingStandardOutputInotifyFileMonitor(
        const std::string &path, const DataReadyCallback &readCallback)
    : d_filePath(path), d_fileFd(openFile(d_filePath)),
      d_dataReadyCallback(readCallback), d_stopRequested(false),
      d_monitoringThread(
          &StreamingStandardOutputInotifyFileMonitor::monitorFile, this),
      d_read_buffer_size(readBufferSizeBytes()),
      d_read_buffer(std::make_unique<char[]>(d_read_buffer_size)),
      d_read_buffer_offset(0)
{
    // Creating an inotify instance:
    d_inotifyInstanceFd = inotify_init();
    if (d_inotifyInstanceFd < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(std::system_error, errno,
                                              std::system_category,
                                              "inotify_init() failed");
    }

    // Adding a watch on the file to track writes to it and detect the event of
    // the writer closing it:
    const auto eventsMask = IN_MODIFY | IN_CLOSE_WRITE;

    d_inotifyWatchFd =
        inotify_add_watch(d_inotifyInstanceFd, d_filePath.c_str(), eventsMask);
    if (d_inotifyWatchFd < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "inotify_add_watch() failed for file [" << d_filePath << "]");
    }
    // This FD will be ready for reading once events are detected.
    // Calling `read()` on it will yield  `struct inotify_event` entries.
}

StreamingStandardOutputInotifyFileMonitor::
    ~StreamingStandardOutputInotifyFileMonitor()
{
    stop();

    inotify_rm_watch(d_inotifyInstanceFd, d_inotifyWatchFd);
    close(d_inotifyInstanceFd);
    close(d_fileFd);
}

void StreamingStandardOutputInotifyFileMonitor::stop()
{
    if (!d_stopRequested) {
        d_stopRequested = true;
        d_monitoringThread.join();
    }
}

int StreamingStandardOutputInotifyFileMonitor::openFile(
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

int StreamingStandardOutputInotifyFileMonitor::waitForInotify() const
{

    // Maximum time that `poll()` will wait.
    const int timeoutMs = 500;
    // (Increasing this makes the delay between `stop()` being called and the
    // monitoring actually stopping longer.)

    const auto maxRetries = 3;
    // We'll retry the call to `poll()` if finding spurious POLLNVAL` values in
    // `revents`.

    for (auto i = 0; i < maxRetries; i++) {
        struct pollfd pollFds;
        pollFds.fd = d_inotifyInstanceFd;
        pollFds.events = POLLIN;
        pollFds.revents = 0;

        const int numFdsReady = poll(&pollFds, 1, timeoutMs);
        if (numFdsReady < 0) {
            const auto errorReason = strerror(errno);
            BUILDBOX_LOG_ERROR("poll() failed: " << errorReason);
            return numFdsReady;
        }

        if (numFdsReady == 0 || (pollFds.revents & POLLIN)) {
            return numFdsReady;
        }
    }

    return -1;
}

bool StreamingStandardOutputInotifyFileMonitor::readInotifyEvents(
    uint32_t *eventsMask) const
{
    // PRE: `d_inotifyInstanceFd` has data available (`waitForInotify()` == 1)

    char inotifyEventBuffer[INOTIFY_BUFFER_SIZE];

    const auto bytesRead =
        read(d_inotifyInstanceFd, inotifyEventBuffer, INOTIFY_BUFFER_SIZE);

    if (bytesRead == -1) {
        const auto errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR(
            "Error reading inotify events from [inotifyInstanceFD="
            << d_inotifyInstanceFd << ", inotifyWatchFD=" << d_inotifyWatchFd
            << "] for [" << d_filePath << "]: " << errorReason);
        return false;
    }

    const auto inotifyStructEventSize = sizeof(struct inotify_event);
    if (static_cast<size_t>(bytesRead) < inotifyStructEventSize) {
        BUILDBOX_LOG_ERROR(
            "Error reading inotify event from [inotifyInstanceFD="
            << d_inotifyInstanceFd << ", inotifyWatchFD=" << d_inotifyWatchFd
            << "] for [" << d_filePath << "]: read " << bytesRead
            << " but expected at least " << inotifyStructEventSize);
    }

    // Reading first event:
    const auto *event1 =
        reinterpret_cast<inotify_event *>(&inotifyEventBuffer[0]);
    *eventsMask = event1->mask;

    // `event1` ends after its `name` field, which is of variable length.
    const auto event1End = inotifyStructEventSize + event1->len;
    if (static_cast<size_t>(bytesRead) > event1End) { // Second event?
        const auto *event2 =
            reinterpret_cast<inotify_event *>(&inotifyEventBuffer[event1End]);

        *eventsMask |= event2->mask;
    }

    BUILDBOX_LOG_TRACE("Read inotify events mask " << eventsMask << " for ["
                                                   << d_filePath << "]");
    return true;
}

bool StreamingStandardOutputInotifyFileMonitor::readFileAndStream()
{
    ssize_t bytesRead;
    do {
        bytesRead = read(d_fileFd, d_read_buffer.get(), d_read_buffer_size);
        BUILDBOX_LOG_TRACE("Read " << bytesRead << " bytes from "
                                   << d_filePath);
        if (bytesRead == -1) {
            const auto errorReason = strerror(errno);
            BUILDBOX_LOG_ERROR("Error reading file " << d_filePath << ": "
                                                     << errorReason);
            return false;
        }

        if (bytesRead > 0) {
            BUILDBOX_LOG_TRACE("Invoking callback with "
                               << bytesRead << " bytes from " << d_filePath);
            d_dataReadyCallback(FileChunk(d_read_buffer.get(),
                                          static_cast<size_t>(bytesRead)));
        }
    } while (bytesRead > 0);

    return true;
}

void StreamingStandardOutputInotifyFileMonitor::monitorFile()
{
    BUILDBOX_LOG_TRACE("Started monitoring thread for " << d_filePath);

    // To avoid missing the initial write to the file when `stop()` is called
    // too soon after initialization, we'll run these number of extra cycles
    // after `d_stopRequested` is set. That way we give the loop a chance to
    // detect, read and stream the changes before shutting down.
    int timeoutCyclesAfterStop = 2;

    while (true) {
        // Poll the inotify instance for changes. If its FD becomes ready, it
        // means that there was a write and/or close event on the file.
        const int numFDsAvailable = waitForInotify();
        if (numFDsAvailable == -1) {
            return;
        }

        if (numFDsAvailable == 0) { // Time out
            BUILDBOX_LOG_TRACE("Inotify event wait timed out for "
                               << d_filePath);

            if (timeoutCyclesAfterStop == 0) {
                BUILDBOX_LOG_TRACE("Stopping monitoring of "
                                   << d_filePath << " after stop requested");
                return;
            }

            if (d_stopRequested) {
                BUILDBOX_LOG_TRACE("Request to stop monitoring "
                                   << d_filePath);
                timeoutCyclesAfterStop--;
            }

            continue;
        }

        BUILDBOX_LOG_TRACE("Inotify event/s available for " << d_filePath);
        uint32_t inotifyeventsMask = 0;
        if (!readInotifyEvents(&inotifyeventsMask)) {
            return;
        }

        const bool updateSuccessfullyStreamed = readFileAndStream();

        const bool fileClosed = (inotifyeventsMask & IN_CLOSE_WRITE);
        if (fileClosed) {
            BUILDBOX_LOG_TRACE("Detected close event for " << d_filePath);
        }

        if (!updateSuccessfullyStreamed || fileClosed) {
            return;
        }
    }
}

} // namespace buildboxcommon
