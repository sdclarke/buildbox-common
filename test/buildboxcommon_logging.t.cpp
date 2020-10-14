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

TEST(LoggingTest, PrintableCommandLineTest)
{
    std::vector<std::string> command{"these", " ", "be", "seperate"};
    std::string result = logging::printableCommandLine(command);
    std::string expected = "these   be seperate";
    EXPECT_EQ(result, expected);
}

TEST(LoggingTest, PrintableCommandLineTestEmpty)
{
    std::vector<std::string> command{};
    std::string result = logging::printableCommandLine(command);
    std::string expected = "";
    EXPECT_EQ(result, expected);
}

TEST(LoggingTest, InitLogging)
{
    const auto programName = "test-program";
    auto &loggerInstance = logging::Logger::getLoggerInstance();
    loggerInstance.initialize(programName);
}
