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
#include <gtest/gtest.h>

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporaryfile.h>

#include <fstream>
#include <functional>

using namespace buildboxcommon;

using namespace testing;

class StreamingStandardOutputInotifyFileMonitorTestFixture
    : public testing::Test {
  protected:
    StreamingStandardOutputInotifyFileMonitorTestFixture()
        : file_monitor(
              monitored_file.strname(),
              std::bind(&StreamingStandardOutputInotifyFileMonitorTestFixture::
                            dataReady,
                        this, std::placeholders::_1))
    {
    }

    void dataReady(
        const StreamingStandardOutputInotifyFileMonitor::FileChunk &chunk)
    {
        if (data_ready_callback) {
            data_ready_callback(chunk);
        }
    }

    buildboxcommon::TemporaryFile monitored_file;
    StreamingStandardOutputInotifyFileMonitor file_monitor;
    StreamingStandardOutputInotifyFileMonitor::DataReadyCallback
        data_ready_callback;
};

TEST_F(StreamingStandardOutputInotifyFileMonitorTestFixture, TestStop)
{
    ASSERT_NO_THROW(file_monitor.stop());
}

TEST_F(StreamingStandardOutputInotifyFileMonitorTestFixture, MonitorEmptyFile)
{
    bool callback_invoked = false;
    StreamingStandardOutputInotifyFileMonitor::DataReadyCallback
        dummy_callback =
            [&callback_invoked](
                const StreamingStandardOutputInotifyFileMonitor::FileChunk &) {
                callback_invoked = true;
            };
    data_ready_callback = dummy_callback;

    ASSERT_NO_THROW(file_monitor.stop());
    ASSERT_FALSE(callback_invoked);
}

TEST_F(StreamingStandardOutputInotifyFileMonitorTestFixture, StopMoreThanOnce)
{
    ASSERT_NO_THROW(file_monitor.stop());
    ASSERT_NO_THROW(file_monitor.stop());
}

TEST_F(StreamingStandardOutputInotifyFileMonitorTestFixture, ReadDataAndStop)
{
    std::string data_read;
    data_ready_callback =
        [&data_read](const StreamingStandardOutputInotifyFileMonitor::FileChunk
                         &chunk) {
            data_read.append(chunk.ptr(), chunk.size());
        };

    // Writing data to a file in two chunks:
    std::ofstream ofs(monitored_file.name(), std::ofstream::out);
    ofs << "Hello, ";
    sleep(2);
    ofs << "world!\n";
    ofs.close();
    sleep(1);

    file_monitor.stop();

    ASSERT_EQ(data_read, "Hello, world!\n");
}

TEST(FileMonitorTest, ReadDataAndDestroy)
{
    buildboxcommon::TemporaryFile file;

    std::string data_read;
    const auto data_ready_callback =
        [&data_read](const StreamingStandardOutputInotifyFileMonitor::FileChunk
                         &chunk) {
            data_read.append(chunk.ptr(), chunk.size());
        };

    {
        StreamingStandardOutputInotifyFileMonitor monitor(file.strname(),
                                                          data_ready_callback);

        std::ofstream ofs(file.strname(), std::ofstream::out);
        ofs << "Hello!";
        ofs.close();
        sleep(2);
    }

    ASSERT_EQ(data_read, "Hello!");
}
