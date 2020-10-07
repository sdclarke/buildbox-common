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

#include <buildboxcommon_reloadtokenauthenticator.h>
#include <buildboxcommon_temporaryfile.h>

#include <fstream>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

TEST(ReloadTokenAuthenticatorTest, ThrowsIfFileDoesNotExist)
{
    EXPECT_THROW(
        ReloadTokenAuthenticator("/dir_no_exist/file_no_exist", "1000"),
        std::runtime_error);
}

TEST(ReloadTokenAuthenticatorTest, NoThrowIfFileDoesExists)
{
    TemporaryFile tmpfile;
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), "1000"));
}

TEST(ReloadTokenAuthenticatorTest, TestReloadTime)
{
    TemporaryFile tmpfile;
    ReloadTokenAuthenticator r(tmpfile.name(), "0");

    struct stat first;
    fstat(tmpfile.fd(), &first);

    usleep(1 * 1000000);
    r.RefreshTokenIfNeeded();

    struct stat second;
    fstat(tmpfile.fd(), &second);

    ASSERT_TRUE(second.st_atime > first.st_atime);
}

TEST(ReloadTokenAuthenticatorTest, TestNoReload)
{
    TemporaryFile tmpfile;
    ReloadTokenAuthenticator r(tmpfile.name(), "5");

    struct stat first;
    fstat(tmpfile.fd(), &first);

    usleep(1 * 1000000);
    r.RefreshTokenIfNeeded();

    struct stat second;
    fstat(tmpfile.fd(), &second);

    ASSERT_FALSE(second.st_atime > first.st_atime);
}

TEST(ReloadTokenAuthenticatorTest, TestNoReloadMinute)
{
    TemporaryFile tmpfile;
    ReloadTokenAuthenticator r(tmpfile.name(), "1M");

    struct stat first;
    fstat(tmpfile.fd(), &first);

    usleep(2 * 1000000);
    r.RefreshTokenIfNeeded();

    struct stat second;
    fstat(tmpfile.fd(), &second);

    ASSERT_FALSE(second.st_atime > first.st_atime);
}

TEST(ReloadTokenAuthenticatorTest, TestNullptrDoesNotThrow)
{
    TemporaryFile tmpfile;
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), nullptr));
}

TEST(ReloadTokenAuthenticatorTest, TestImproperSuffix)
{
    TemporaryFile tmpfile;
    EXPECT_THROW(ReloadTokenAuthenticator(tmpfile.name(), "1S"),
                 std::invalid_argument);
    EXPECT_THROW(ReloadTokenAuthenticator(tmpfile.name(), "10D"),
                 std::invalid_argument);
    EXPECT_THROW(ReloadTokenAuthenticator(tmpfile.name(), "1W"),
                 std::invalid_argument);
}

TEST(ReloadTokenAuthenticatorTest, TestWithJustSuffix)
{
    TemporaryFile tmpfile;
    EXPECT_THROW(ReloadTokenAuthenticator(tmpfile.name(), "M"),
                 std::invalid_argument);
}

TEST(ReloadTokenAuthenticatorTest, TestExtraLongSuffix)
{
    TemporaryFile tmpfile;
    EXPECT_THROW(ReloadTokenAuthenticator(tmpfile.name(), "10Minutes"),
                 std::invalid_argument);
}

TEST(ReloadTokenAuthenticatorTest, TestValidSuffix)
{
    TemporaryFile tmpfile;
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), "2M"));
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), "30H"));
}

TEST(ReloadTokenAuthenticatorTest, TestValidSuffixLowerCase)
{
    TemporaryFile tmpfile;
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), "20m"));
    EXPECT_NO_THROW(ReloadTokenAuthenticator(tmpfile.name(), "2h"));
}
