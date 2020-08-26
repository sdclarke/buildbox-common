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
    "--cas-retry-delay=500"
};

const char *argvTestRequiredOnly[] = {
    "/some/path/to/some_program.tsk",
    "--cas-remote=http://127.0.0.1:50011"
};
// clang-format on

TEST(ConnectionOptionsCommandLineTest, BasicTest)
{
    ConnectionOptionsCommandLine spec("CAS", "cas-");
    CommandLine commandLine(spec.spec());

    EXPECT_TRUE(
        commandLine.parse(sizeof(argvTest) / sizeof(const char *), argvTest));

    EXPECT_FALSE(ConnectionOptionsCommandLine::configureClient(
        commandLine, "cas-", nullptr));

    ConnectionOptions client;
    EXPECT_TRUE(ConnectionOptionsCommandLine::configureClient(
        commandLine, "cas-", &client));

    ASSERT_TRUE(client.d_url != nullptr);
    EXPECT_STREQ("http://127.0.0.1:50011", client.d_url);

    ASSERT_TRUE(client.d_instanceName != nullptr);
    EXPECT_STREQ("dev", client.d_instanceName);

    ASSERT_TRUE(client.d_serverCertPath != nullptr);
    EXPECT_STREQ("my-server-cert", client.d_serverCertPath);

    ASSERT_TRUE(client.d_clientKeyPath != nullptr);
    EXPECT_STREQ("my-client-key", client.d_clientKeyPath);

    ASSERT_TRUE(client.d_clientCertPath != nullptr);
    EXPECT_STREQ("my-client-cert", client.d_clientCertPath);

    ASSERT_TRUE(client.d_accessTokenPath != nullptr);
    EXPECT_STREQ("my-access-token", client.d_accessTokenPath);

    EXPECT_TRUE(client.d_useGoogleApiAuth);

    ASSERT_TRUE(client.d_retryLimit != nullptr);
    EXPECT_STREQ("10", client.d_retryLimit);

    ASSERT_TRUE(client.d_retryDelay != nullptr);
    EXPECT_STREQ("500", client.d_retryDelay);
}

TEST(ConnectionOptionsCommandLineTest, RequiredTest)
{
    ConnectionOptionsCommandLine spec("CAS", "cas-");
    CommandLine commandLine(spec.spec());

    EXPECT_TRUE(
        commandLine.parse(sizeof(argvTestRequiredOnly) / sizeof(const char *),
                          argvTestRequiredOnly));

    ConnectionOptions client;
    EXPECT_TRUE(ConnectionOptionsCommandLine::configureClient(
        commandLine, "cas-", &client));

    ASSERT_TRUE(client.d_url != nullptr);
    EXPECT_STREQ("http://127.0.0.1:50011", client.d_url);

    ASSERT_TRUE(client.d_instanceName != nullptr);
    EXPECT_STREQ("", client.d_instanceName);

    EXPECT_TRUE(client.d_serverCertPath == nullptr);
    EXPECT_TRUE(client.d_clientKeyPath == nullptr);
    EXPECT_TRUE(client.d_clientCertPath == nullptr);
    EXPECT_TRUE(client.d_accessTokenPath == nullptr);
    EXPECT_FALSE(client.d_useGoogleApiAuth);

    ASSERT_TRUE(client.d_retryLimit != nullptr);
    EXPECT_STREQ("4", client.d_retryLimit);

    ASSERT_TRUE(client.d_retryDelay != nullptr);
    EXPECT_STREQ("1000", client.d_retryDelay);
}
