/*
 * Copyright 2018 Bloomberg Finance LP
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

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(ConnectionOptionsTest, DefaultsToNullptrs)
{
    ConnectionOptions opts;
    EXPECT_EQ(opts.d_url, nullptr);
    EXPECT_EQ(opts.d_instanceName, nullptr);
    EXPECT_EQ(opts.d_serverCert, nullptr);
    EXPECT_EQ(opts.d_clientKey, nullptr);
    EXPECT_EQ(opts.d_clientCert, nullptr);
}

TEST(ConnectionOptionsTest, ParseArgIgnoresInvalidArgs)
{
    ConnectionOptions opts;

    ASSERT_FALSE(opts.parseArg(nullptr));
    ASSERT_FALSE(opts.parseArg(""));
    ASSERT_FALSE(opts.parseArg("test"));
    ASSERT_FALSE(opts.parseArg("--invalid-flag"));
    ASSERT_FALSE(opts.parseArg("--invalid-argument=hello"));
    ASSERT_FALSE(opts.parseArg("--remote"));

    EXPECT_EQ(opts.d_url, nullptr);
    EXPECT_EQ(opts.d_serverCert, nullptr);
    EXPECT_EQ(opts.d_clientKey, nullptr);
    EXPECT_EQ(opts.d_clientCert, nullptr);
}

TEST(ConnectionOptionsTest, ParseArgSimple)
{
    ConnectionOptions opts;

    ASSERT_TRUE(opts.parseArg("--remote=abc"));
    EXPECT_STREQ(opts.d_url, "abc");

    ASSERT_TRUE(opts.parseArg("--instance=testingInstances/instance1"));

    ASSERT_TRUE(opts.parseArg("--server-cert=defg"));
    EXPECT_STREQ(opts.d_serverCert, "defg");

    ASSERT_TRUE(opts.parseArg("--client-key=h"));
    EXPECT_STREQ(opts.d_clientKey, "h");

    ASSERT_TRUE(opts.parseArg("--client-cert="));
    EXPECT_STREQ(opts.d_clientCert, "");

    EXPECT_STREQ(opts.d_url, "abc");
    EXPECT_STREQ(opts.d_instanceName, "testingInstances/instance1");
    EXPECT_STREQ(opts.d_serverCert, "defg");
    EXPECT_STREQ(opts.d_clientKey, "h");
    EXPECT_STREQ(opts.d_clientCert, "");
}

TEST(ConnectionOptionsTest, ParseArgIgnoresWrongPrefix)
{
    ConnectionOptions opts;

    ASSERT_FALSE(opts.parseArg("--cas-remote=test"));
    ASSERT_FALSE(opts.parseArg("--remote=test", "cas-"));
    ASSERT_FALSE(opts.parseArg("--abc-remote=test", "cas-"));

    EXPECT_EQ(opts.d_url, nullptr);
    EXPECT_EQ(opts.d_serverCert, nullptr);
    EXPECT_EQ(opts.d_clientKey, nullptr);
    EXPECT_EQ(opts.d_clientCert, nullptr);
}

TEST(ConnectionOptionsTest, ParseArgWorksWithPrefix)
{
    ConnectionOptions opts;

    ASSERT_TRUE(opts.parseArg("--cas-remote=abc", "cas-"));
    EXPECT_STREQ(opts.d_url, "abc");

    ASSERT_TRUE(opts.parseArg("--cas-instance=RemoteInstanceName", "cas-"));
    EXPECT_STREQ(opts.d_instanceName, "RemoteInstanceName");

    ASSERT_TRUE(opts.parseArg("--cas-server-cert=defg", "cas-"));
    EXPECT_STREQ(opts.d_serverCert, "defg");

    ASSERT_TRUE(opts.parseArg("--cas-client-key=h", "cas-"));
    EXPECT_STREQ(opts.d_clientKey, "h");

    ASSERT_TRUE(opts.parseArg("--cas-client-cert=", "cas-"));
    EXPECT_STREQ(opts.d_clientCert, "");

    EXPECT_STREQ(opts.d_url, "abc");
    EXPECT_STREQ(opts.d_serverCert, "defg");
    EXPECT_STREQ(opts.d_clientKey, "h");
    EXPECT_STREQ(opts.d_clientCert, "");
}

TEST(ConnectionOptionsTest, PutArgsEmpty)
{
    ConnectionOptions opts;
    std::vector<std::string> result;

    opts.putArgs(&result);

    opts.putArgs(&result, "cas-");
    std::vector<std::string> expected = {
        "--retry-limit=0", "--retry-delay=100", "--cas-retry-limit=0",
        "--cas-retry-delay=100"};
    EXPECT_EQ(result, expected);
}

TEST(ConnectionOptionsTest, PutArgsFull)
{
    ConnectionOptions opts;
    opts.d_url = "http://example.com/";
    opts.d_instanceName = "instanceA";
    opts.d_serverCert = "abc";
    opts.d_clientKey = "defg";
    opts.d_clientCert = "";
    opts.d_retryLimit = "2";
    opts.d_retryDelay = "200";

    std::vector<std::string> result;

    opts.putArgs(&result);

    std::vector<std::string> expected = {"--remote=http://example.com/",
                                         "--instance=instanceA",
                                         "--server-cert=abc",
                                         "--client-key=defg",
                                         "--client-cert=",
                                         "--retry-limit=2",
                                         "--retry-delay=200"};
    EXPECT_EQ(result, expected);

    opts.putArgs(&result, "cas-");
    expected.push_back("--cas-remote=http://example.com/");
    expected.push_back("--cas-instance=instanceA");
    expected.push_back("--cas-server-cert=abc");
    expected.push_back("--cas-client-key=defg");
    expected.push_back("--cas-client-cert=");
    expected.push_back("--cas-retry-limit=2");
    expected.push_back("--cas-retry-delay=200");
    EXPECT_EQ(result, expected);
}

TEST(ConnectionOptionsTest, ArgHelpDoesntCrash)
{
    ConnectionOptions::printArgHelp(0);
    ConnectionOptions::printArgHelp(40, "Bots", "bots-");
}
