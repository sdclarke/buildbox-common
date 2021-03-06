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

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_connectionoptions_commandline.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;
using namespace testing;

// clang-format off
const char *argvTest[] = {
    "/some/path/to/some_program.tsk",
    "--cas-remote=http://127.0.0.1:50011",
    "--cas-instance=dev",
    "--cas-server-cert=my-server-cert",
    "--cas-client-key=my-client-key",
    "--cas-client-cert=my-client-cert",
    "--cas-access-token=my-access-token",
    "--cas-googleapi-auth=true",
    "--cas-retry-limit=10",
    "--cas-retry-delay=500",
    "--cas-load-balancing-policy=round_robin"
};

const char *argvTestDefaults[] = {
    "/some/path/to/some_program.tsk",
    "--cas-remote=http://127.0.0.1:50011"
};

const char *argvTestRequired[] = {
    "/some/path/to/some_program.tsk"
};
// clang-format on

TEST(ConnectionOptionsCommandLineTest, Test)
{
    ConnectionOptionsCommandLine spec("CAS", "cas-");
    CommandLine commandLine(spec.spec());

    EXPECT_TRUE(
        commandLine.parse(sizeof(argvTest) / sizeof(const char *), argvTest));

    EXPECT_FALSE(ConnectionOptionsCommandLine::configureChannel(
        commandLine, "cas-", nullptr));

    ConnectionOptions channel;
    EXPECT_TRUE(ConnectionOptionsCommandLine::configureChannel(
        commandLine, "cas-", &channel));

    ASSERT_TRUE(channel.d_url != nullptr);
    EXPECT_STREQ("http://127.0.0.1:50011", channel.d_url);

    ASSERT_TRUE(channel.d_instanceName != nullptr);
    EXPECT_STREQ("dev", channel.d_instanceName);

    ASSERT_TRUE(channel.d_serverCertPath != nullptr);
    EXPECT_STREQ("my-server-cert", channel.d_serverCertPath);

    ASSERT_TRUE(channel.d_clientKeyPath != nullptr);
    EXPECT_STREQ("my-client-key", channel.d_clientKeyPath);

    ASSERT_TRUE(channel.d_clientCertPath != nullptr);
    EXPECT_STREQ("my-client-cert", channel.d_clientCertPath);

    ASSERT_TRUE(channel.d_accessTokenPath != nullptr);
    EXPECT_STREQ("my-access-token", channel.d_accessTokenPath);

    EXPECT_TRUE(channel.d_useGoogleApiAuth);

    ASSERT_TRUE(channel.d_retryLimit != nullptr);
    EXPECT_STREQ("10", channel.d_retryLimit);

    ASSERT_TRUE(channel.d_retryDelay != nullptr);
    EXPECT_STREQ("500", channel.d_retryDelay);

    ASSERT_TRUE(channel.d_loadBalancingPolicy != nullptr);
    EXPECT_STREQ("round_robin", channel.d_loadBalancingPolicy);
}

TEST(ConnectionOptionsCommandLineTest, TestDefaults)
{
    ConnectionOptionsCommandLine spec("CAS", "cas-");
    CommandLine commandLine(spec.spec());

    EXPECT_TRUE(commandLine.parse(
        sizeof(argvTestDefaults) / sizeof(const char *), argvTestDefaults));

    ConnectionOptions channel;
    EXPECT_TRUE(ConnectionOptionsCommandLine::configureChannel(
        commandLine, "cas-", &channel));

    ASSERT_TRUE(channel.d_url != nullptr);
    EXPECT_STREQ("http://127.0.0.1:50011", channel.d_url);

    // test default values
    ASSERT_TRUE(channel.d_instanceName != nullptr);
    EXPECT_STREQ("", channel.d_instanceName);
    EXPECT_FALSE(channel.d_useGoogleApiAuth);
    ASSERT_TRUE(channel.d_retryLimit != nullptr);
    EXPECT_STREQ("4", channel.d_retryLimit);
    ASSERT_TRUE(channel.d_retryDelay != nullptr);
    EXPECT_STREQ("1000", channel.d_retryDelay);

    // untouched
    EXPECT_TRUE(channel.d_serverCertPath == nullptr);
    EXPECT_TRUE(channel.d_clientKeyPath == nullptr);
    EXPECT_TRUE(channel.d_clientCertPath == nullptr);
    EXPECT_TRUE(channel.d_accessTokenPath == nullptr);
}

TEST(ConnectionOptionsCommandLineTest, TestRequired)
{
    ConnectionOptionsCommandLine spec("CAS", "cas-", true);
    CommandLine commandLine(spec.spec());

    EXPECT_FALSE(commandLine.parse(
        sizeof(argvTestRequired) / sizeof(const char *), argvTestRequired));
}
