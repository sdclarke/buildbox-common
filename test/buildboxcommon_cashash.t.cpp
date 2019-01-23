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

#include <buildboxcommon_cashash.h>

#include <fcntl.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(CASHashTest, EmptyString)
{
    const Digest d = CASHash::hash("");
    EXPECT_EQ(
        d.hash(),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(d.size_bytes(), 0);
}

TEST(CASHashTest, NonEmptyString)
{
    const Digest d = CASHash::hash("test");
    EXPECT_EQ(
        d.hash(),
        "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
    EXPECT_EQ(d.size_bytes(), 4);
}

TEST(CASHashTest, File)
{
    int fd = open("test.txt", O_RDONLY);
    const Digest d = CASHash::hash(fd);
    EXPECT_EQ(
        d.hash(),
        "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
    EXPECT_EQ(d.size_bytes(), 4);
    close(fd);
}
