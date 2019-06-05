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

#include <buildboxcommon_protos.h>

#include <buildboxcommon_cashash.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(ProtosHeaderTest, DigestComparisonEqual)
{
    Digest d1, d2;

    d1.set_hash("hash");
    d1.set_size_bytes(1024);

    d2.CopyFrom(d1);

    ASSERT_TRUE(d1 == d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferentHash)
{
    Digest d1, d2;

    d1.set_hash("hash1");
    d2.set_hash("hash2");

    d1.set_size_bytes(1024);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferentSize)
{
    Digest d1, d2;

    d1.set_hash("hash");
    d2.set_hash("hash");

    d1.set_size_bytes(100);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferent)
{
    Digest d1, d2;

    d1.set_hash("hash1");
    d2.set_hash("hash2");

    d1.set_size_bytes(100);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
}

TEST(ProtosHeaderTest, DigestToString)
{
    const std::string data = "This is some content to hash.";
    const Digest digest = buildboxcommon::CASHash::hash(data);

    std::string expected_output =
        digest.hash() + "/" + std::to_string(digest.size_bytes());

    // toString():
    ASSERT_EQ(toString(digest), expected_output);

    // Operator `<<`:
    std::stringstream output;
    output << digest;
    ASSERT_EQ(output.str(), expected_output);
}
