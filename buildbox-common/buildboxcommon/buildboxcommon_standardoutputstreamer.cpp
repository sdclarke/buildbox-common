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

#include <buildboxcommon_standardoutputstreamer.h>

#include <buildboxcommon_logging.h>

#include <functional>

namespace buildboxcommon {

StandardOutputStreamer::StandardOutputStreamer(
    const std::string &path, const ConnectionOptions &connectionOptions,
    const std::string &resourceName)
    : d_filePath(path), d_url(connectionOptions.d_url),
      d_resourceName(resourceName),
      d_logstreamWriter(d_resourceName, connectionOptions),
      d_fileMonitor(d_filePath,
                    std::bind(&StandardOutputStreamer::streamLogChunk, this,
                              std::placeholders::_1))
{
}

StandardOutputStreamer::~StandardOutputStreamer()
{
    if (!d_stopRequested) {
        BUILDBOX_LOG_WARNING("Destroying `StandardOutputStreamer` instance "
                             "without invoking `stop()`, outputs may be lost");
    }
}

bool StandardOutputStreamer::stop()
{
    if (d_stopRequested) {
        return false;
    }
    d_stopRequested = true;

    BUILDBOX_LOG_DEBUG("Stopping the monitoring of ["
                       << d_filePath << "] and committing the log");

    d_fileMonitor.stop();
    return d_logstreamWriter.commit();
}

void StandardOutputStreamer::streamLogChunk(
    const StreamingStandardOutputFileMonitor::FileChunk &chunk)
{
    BUILDBOX_LOG_DEBUG("File monitor reported ["
                       << d_filePath << "] has " << chunk.size()
                       << " bytes available, streaming to "
                       << "[" << d_url << "/" << d_resourceName << "]");

    const bool write_suceeded =
        d_logstreamWriter.write(std::string(chunk.ptr(), chunk.size()));

    if (!write_suceeded) {
        BUILDBOX_LOG_DEBUG(
            "`Write()` call failed, stopping the monitoring thread");
        d_stopRequested = true;
        d_fileMonitor.stop();
    }
}

} // namespace buildboxcommon
