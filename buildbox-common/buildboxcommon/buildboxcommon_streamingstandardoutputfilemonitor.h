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

#ifndef INCLUDED_BUILDBOXCOMMON_STREAMINGSTANDARDOUTPUTFILEMONITOR
#define INCLUDED_BUILDBOXCOMMON_STREAMINGSTANDARDOUTPUTFILEMONITOR

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

namespace buildboxcommon {
class StreamingStandardOutputFileMonitor {
    /*
     * This class allows to read a file that is being written by a process that
     * is redirecting its standard output (stdout/stderr) to it. As the file
     * is modified, it invokes a provided callback with the newly-appended
     * data.
     *
     * For that it spawns a separate thread, which will keep monitoring the
     * file until either the object is destructed or the `stop()` method is
     * called.
     */

  public:
    struct FileChunk {
        FileChunk(const char *ptr, const size_t size)
            : d_ptr(ptr), d_size(size)
        {
        }

        const char *ptr() const { return d_ptr; }
        const size_t &size() const { return d_size; }

      private:
        const char *const d_ptr;
        const size_t d_size;
    };

    // Callback invoked with the data that is made available.
    // Note that this will cause the monitor to block until its return, so no
    // new data will be read until the callback is done.
    typedef std::function<void(const FileChunk &)> DataReadyCallback;

    virtual ~StreamingStandardOutputFileMonitor(){};

    // Stop the monitoring thread.
    // To not lose any data, the caller should make sure that the reader has
    // stopped writing to and closed the file.
    virtual void stop() = 0;

  protected:
    static int openFile(const std::string &path);
    static size_t readBufferSizeBytes();
};
} // namespace buildboxcommon

#endif
