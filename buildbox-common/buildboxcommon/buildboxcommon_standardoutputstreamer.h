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

#ifndef INCLUDED_BUILDBOXCOMMON_STANDARDOUTPUTSTREAMER
#define INCLUDED_BUILDBOXCOMMON_STANDARDOUTPUTSTREAMER

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_logstreamwriter.h>
#include <buildboxcommon_streamingstandardoutputfilemonitor.h>

#include <string>

namespace buildboxcommon {

class StandardOutputStreamer {
  public:
    /* Given a path to a file, a ByteStream endpoint and a resource name,
     * stream the contents of that file as they are updated.
     *
     * The file must be written in an append-only manner.
     */
    explicit StandardOutputStreamer(const std::string &path,
                                    const ConnectionOptions &connectionOptions,
                                    const std::string &resourceName);

    ~StandardOutputStreamer();

    // Stop monitoring the file, issue a `finish_write` request and close the
    // connection.
    // (The monitor might be already stopped by the time this method is called
    // due to a `write()` request failure.)
    // Returns whether the data was completely transfered and commited.
    bool stop();

  private:
    void
    streamLogChunk(const StreamingStandardOutputFileMonitor::FileChunk &chunk);

    const std::string d_filePath;
    const std::string d_url;
    const std::string d_resourceName;

    LogStreamWriter d_logstreamWriter;
    StreamingStandardOutputFileMonitor d_fileMonitor;

    bool d_stopRequested;
};
} // namespace buildboxcommon

#endif
