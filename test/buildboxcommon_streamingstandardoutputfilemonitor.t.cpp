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
#include <gtest/gtest.h>

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporaryfile.h>

#include <fstream>
#include <functional>

using namespace buildboxcommon;

using namespace testing;

class StreamingStandardOutputFileMonitorTestFixture : public testing::Test {
  protected:
    StreamingStandardOutputFileMonitorTestFixture()
        : file_monitor(
              monitored_file.strname(),
              std::bind(
                  &StreamingStandardOutputFileMonitorTestFixture::dataReady,
                  this, std::placeholders::_1))
    {
    }

    void dataReady(const StreamingStandardOutputFileMonitor::FileChunk &chunk)
    {
        if (data_ready_callback) {
            data_ready_callback(chunk);
        }
    }

    buildboxcommon::TemporaryFile monitored_file;
    StreamingStandardOutputFileMonitor file_monitor;
    StreamingStandardOutputFileMonitor::DataReadyCallback data_ready_callback;
};

TEST_F(StreamingStandardOutputFileMonitorTestFixture, TestStop)
{
    ASSERT_NO_THROW(file_monitor.stop());
}

TEST_F(StreamingStandardOutputFileMonitorTestFixture, MonitorEmptyFile)
{
    bool callback_invoked = false;
    StreamingStandardOutputFileMonitor::DataReadyCallback dummy_callback =
        [&callback_invoked](
            const StreamingStandardOutputFileMonitor::FileChunk &) {
            callback_invoked = true;
        };
    data_ready_callback = dummy_callback;

    ASSERT_NO_THROW(file_monitor.stop());
    ASSERT_FALSE(callback_invoked);
}

TEST_F(StreamingStandardOutputFileMonitorTestFixture, StopMoreThanOnce)
{
    ASSERT_NO_THROW(file_monitor.stop());
    ASSERT_NO_THROW(file_monitor.stop());
}

TEST_F(StreamingStandardOutputFileMonitorTestFixture, ReadDataAndStop)
{
    std::string data_read;
    data_ready_callback =
        [&data_read](
            const StreamingStandardOutputFileMonitor::FileChunk &chunk) {
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
    {
        StreamingStandardOutputFileMonitor monitor(
            file.strname(),
            [&data_read](
                const StreamingStandardOutputFileMonitor::FileChunk &chunk) {
                data_read.append(chunk.ptr(), chunk.size());
            });

        std::ofstream ofs(file.strname(), std::ofstream::out);
        ofs << "Hello!";
        ofs.close();
        sleep(2);
    }

    ASSERT_EQ(data_read, "Hello!");
}
