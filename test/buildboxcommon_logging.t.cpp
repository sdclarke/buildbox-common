/*
 * Copyright 2019 Bloomberg Finance LP
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

#include <buildboxcommon_logging.h>
#include <buildboxcommon_temporaryfile.h>

#include <gtest/gtest.h>

#include <fstream>

using namespace buildboxcommon;

class LoggingFixture : public ::testing::Test {
  protected:
    LoggingFixture()
    {
        /**
         * Explicitly set the stream each test so output redirection
         * from previous tests doesn't affect the current test
         */
        BUILDBOX_LOG_SET_STREAM(std::clog);
        BUILDBOX_LOG_DISABLE_PREFIX();
    }
};

TEST_F(LoggingFixture, SimpleLogLine)
{
    testing::internal::CaptureStderr();
    int num = 123;
    BUILDBOX_LOG_INFO("testing testing " << num);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(output, "[INFO] testing testing 123\n");
}

TEST_F(LoggingFixture, TestDebugLevel)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::DEBUG);
    BUILDBOX_LOG_DEBUG("this is a debug line");
    BUILDBOX_LOG_INFO("this is an info line");
    BUILDBOX_LOG_ERROR("this is an error line");
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(output, "[DEBUG] this is a debug line\n[INFO] this is an info "
                      "line\n[ERROR] this is an error line\n");
}

TEST_F(LoggingFixture, TestInfoLevel)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::INFO);
    BUILDBOX_LOG_DEBUG("this is a debug line");
    BUILDBOX_LOG_INFO("this is an info line");
    BUILDBOX_LOG_ERROR("this is an error line");
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(output,
              "[INFO] this is an info line\n[ERROR] this is an error line\n");
}

TEST_F(LoggingFixture, TestErrorLevel)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::ERROR);
    BUILDBOX_LOG_DEBUG("this is a debug line");
    BUILDBOX_LOG_INFO("this is an info line");
    BUILDBOX_LOG_ERROR("this is an error line");
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(output, "[ERROR] this is an error line\n");
}

TEST_F(LoggingFixture, TestSwitchingLogLevels)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::DEBUG);
    BUILDBOX_LOG_DEBUG("testing123");
    BUILDBOX_LOG_INFO("testing123");
    BUILDBOX_LOG_ERROR("testing123");
    BUILDBOX_LOG_SET_LEVEL(LogLevel::INFO);
    BUILDBOX_LOG_DEBUG("testing456");
    BUILDBOX_LOG_INFO("testing456");
    BUILDBOX_LOG_ERROR("testing456");
    BUILDBOX_LOG_SET_LEVEL(LogLevel::ERROR);
    BUILDBOX_LOG_DEBUG("testing789");
    BUILDBOX_LOG_INFO("testing789");
    BUILDBOX_LOG_ERROR("testing789");
    std::string output = testing::internal::GetCapturedStderr();
    std::string expected = "[DEBUG] testing123\n"
                           "[INFO] testing123\n"
                           "[ERROR] testing123\n"
                           "[INFO] testing456\n"
                           "[ERROR] testing456\n"
                           "[ERROR] testing789\n";
    EXPECT_EQ(output, expected);
}

std::string get_file_contents(std::string filename)
{
    std::ifstream in(filename);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

TEST_F(LoggingFixture, SimpleLogToFileTest)
{
    TemporaryFile tmpfile;
    BUILDBOX_LOG_SET_FILE(tmpfile.name());
    BUILDBOX_LOG_ERROR("testing testing");
    EXPECT_EQ(get_file_contents(tmpfile.name()), "[ERROR] testing testing\n");
}

TEST_F(LoggingFixture, SwitchFiles)
{
    TemporaryFile tmpfile;
    TemporaryFile tmpfile2;
    BUILDBOX_LOG_SET_FILE(tmpfile.name());
    BUILDBOX_LOG_ERROR("testing123");
    BUILDBOX_LOG_SET_FILE(tmpfile2.name());
    BUILDBOX_LOG_ERROR("testing456");

    EXPECT_EQ(get_file_contents(tmpfile.name()), "[ERROR] testing123\n");
    EXPECT_EQ(get_file_contents(tmpfile2.name()), "[ERROR] testing456\n");
}

TEST_F(LoggingFixture, SwitchStreams)
{
    testing::internal::CaptureStderr();
    testing::internal::CaptureStdout();
    BUILDBOX_LOG_ERROR("This is a message to std::clog.");
    BUILDBOX_LOG_SET_STREAM(std::cout);
    BUILDBOX_LOG_ERROR("This is a message to std::cout.");

    std::string clogOutput = testing::internal::GetCapturedStderr();
    EXPECT_EQ(clogOutput, "[ERROR] This is a message to std::clog.\n");
    std::string coutOutput = testing::internal::GetCapturedStdout();
    EXPECT_EQ(coutOutput, "[ERROR] This is a message to std::cout.\n");
}

TEST_F(LoggingFixture, SwitchStreamToFile)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::ERROR);
    TemporaryFile tmpfile;
    BUILDBOX_LOG_ERROR("This is a message to std::clog.");
    BUILDBOX_LOG_SET_FILE(tmpfile.name());
    BUILDBOX_LOG_ERROR("This is a message to a log file.");

    std::string clogOutput = testing::internal::GetCapturedStderr();
    EXPECT_EQ(clogOutput, "[ERROR] This is a message to std::clog.\n");
    EXPECT_EQ(get_file_contents(tmpfile.name()),
              "[ERROR] This is a message to a log file.\n");
}

TEST_F(LoggingFixture, SwitchFileToStream)
{
    testing::internal::CaptureStderr();
    BUILDBOX_LOG_SET_LEVEL(LogLevel::ERROR);
    TemporaryFile tmpfile;
    BUILDBOX_LOG_SET_FILE(tmpfile.name());
    BUILDBOX_LOG_ERROR("This is a message to a log file.");
    BUILDBOX_LOG_SET_STREAM(std::clog);
    BUILDBOX_LOG_ERROR("This is a message to std::clog.");

    EXPECT_EQ(get_file_contents(tmpfile.name()),
              "[ERROR] This is a message to a log file.\n");
    std::string clogOutput = testing::internal::GetCapturedStderr();
    EXPECT_EQ(clogOutput, "[ERROR] This is a message to std::clog.\n");
}
