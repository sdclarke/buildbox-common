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
#ifndef INCLUDED_BUILDBOXCOMMON_STREAMINGSTANDARDOUTPUTINOTIFYFILEMONITOR
#define INCLUDED_BUILDBOXCOMMON_STREAMINGSTANDARDOUTPUTINOTIFYFILEMONITOR

#ifdef FILEMONITOR_USE_INOTIFY

#include <buildboxcommon_streamingstandardoutputfilemonitor.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <sys/inotify.h>

namespace buildboxcommon {
class StreamingStandardOutputInotifyFileMonitor final
    : public StreamingStandardOutputFileMonitor {
    /*
     * This class allows to read a file that is being written by a process that
     * is redirecting its standard output (stdout/stderr) to it. As the file
     * is modified, it invokes a provided callback with the newly-appended
     * data.
     *
     * For that it spawns a separate thread, which will keep monitoring the
     * file until either the object is destructed or the `stop()` method is
     * called.
     *
     * In order to detect changes to the file, it uses the inotify API (Linux
     * only). That API allows to set up a "watch" on a file with a list of
     * events. That watch is a file descriptor that can be `read()` to obtain
     * information about the event, and `poll()` can be used to detect whether
     * new events are available for reading.
     *
     * In addition to MODIFY events, this will also listen for the
     * closing of the file. Once a file is closed by its only writer, we can be
     * sure that reaching EOF means that the monitoring can stop, since no more
     * data will be added.
     */

  public:
    // Spawns a thread that monitors a file for changes and reads it as it is
    // being written. When new data is available invokes the given callback.
    StreamingStandardOutputInotifyFileMonitor(
        const std::string &path, const DataReadyCallback &dataReadyCallback);

    ~StreamingStandardOutputInotifyFileMonitor() override;

    // Stop the monitoring thread.
    // To not lose any data, the caller should make sure that the reader has
    // stopped writing to and closed the file.
    void stop() override;

    // Prevent copies of instances.
    StreamingStandardOutputInotifyFileMonitor(
        const StreamingStandardOutputInotifyFileMonitor &) = delete;
    StreamingStandardOutputInotifyFileMonitor &
    operator=(StreamingStandardOutputInotifyFileMonitor const &) = delete;

  private:
    const std::string d_filePath;
    int d_fileFd;

    int d_inotifyInstanceFd;
    int d_inotifyWatchFd;

    const DataReadyCallback d_dataReadyCallback;
    std::atomic<bool> d_stopRequested;
    std::thread d_monitoringThread;

    // Thread that performs the monitoring and, when data is available, reads
    // from the file and invokes the callback.
    // It will stop and return only when `d_stop_requested` is set.
    void monitorFile();

    // Call poll() on `d_inotifyInstanceFd` and return 1 if there's an event
    // ready to read, 0 if it timed out, -1 on errors.
    int waitForInotify() const;

    // Read the inotify events available from `d_inotifyInstanceFd`. (If no
    // data is ready it will block, so it should be called after
    // `waitForInotify()`).
    // If successful, sets the eventMask ORing the event masks and returns
    // true. On errors returns false.
    bool readInotifyEvents(uint32_t *eventsMask) const;

    const size_t d_read_buffer_size;
    std::unique_ptr<char[]> d_read_buffer;
    size_t d_read_buffer_offset;

    // Read the changes in the file and invoke the callback function with it.
    bool readFileAndStream();
};
} // namespace buildboxcommon
#endif

#endif
