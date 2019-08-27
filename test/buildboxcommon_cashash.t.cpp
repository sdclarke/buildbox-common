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

#include <buildboxcommon_temporaryfile.h>
#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

using namespace buildboxcommon;

using namespace std::literals::string_literals;
const std::string TEST_STRING =
    "This is a sample blob to hash. \0 It contains some NUL characters "
    "\0."s;

TEST(CASHashTest, DefaultFunction)
{
    ASSERT_EQ(CASHash::digestFunction(),
              DigestFunction_Value::DigestFunction_Value_SHA256);

    const std::string data = "Hello, world!";

    const Digest d1 =
        DigestGenerator(DigestFunction_Value::DigestFunction_Value_SHA256)
            .hash(data);
    const Digest d2 = DigestGenerator().hash(data);

    ASSERT_EQ(d1, d2);
}

TEST(CASHashTest, EmptyString)
{

    const Digest d =
        DigestGenerator(DigestFunction_Value::DigestFunction_Value_SHA256)
            .hash("");
    EXPECT_EQ(
        d.hash(),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(d.size_bytes(), 0);
}

TEST(CASHashTest, TestNonEmptyString)
{

    const Digest d1 = CASHash::hash(TEST_STRING);

    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_SHA256);
    const Digest d2 = dg.hash(TEST_STRING);

    const std::string expected_sha256_hash =
        "b1c4daf6e3812505064c07f1ad0b1d6693d93b1b28c452e55ad17e38c30e89aa";

    EXPECT_EQ(d1.hash(), expected_sha256_hash);
    EXPECT_EQ(d1.size_bytes(), TEST_STRING.size());
    EXPECT_EQ(d1, d2);
}

TEST(DigestGeneratorTest, TestStringMD5)
{
    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_MD5);
    const Digest d = dg.hash(TEST_STRING);

    const std::string expected_md5_string = "c1ad80398f865c700449c073bd0a8358";

    EXPECT_EQ(d.hash(), expected_md5_string);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha1)
{
    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_SHA1);
    const Digest d = dg.hash(TEST_STRING);

    const std::string expected_sha1_hash =
        "716e65700ad0e969cca29ec2259fa526e4bdb129";

    EXPECT_EQ(d.hash(), expected_sha1_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha256)
{
    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_SHA256);
    const Digest d = dg.hash(TEST_STRING);

    const std::string expected_sha256_hash =
        "b1c4daf6e3812505064c07f1ad0b1d6693d93b1b28c452e55ad17e38c30e89aa";

    EXPECT_EQ(d.hash(), expected_sha256_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha384)
{
    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_SHA384);
    const Digest d = dg.hash(TEST_STRING);

    const std::string expected_sha384_hash =
        "614589fe6e8bfd0e5a78e6819e439965364ec3af3a7482b69dd62e4ba47d82b5e305c"
        "b609d529164c794ba2b98e0279b";

    EXPECT_EQ(d.hash(), expected_sha384_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha512)
{
    DigestGenerator dg =
        DigestGenerator(DigestFunction::Value::DigestFunction_Value_SHA512);
    const Digest d = dg.hash(TEST_STRING);

    const std::string expected_sha512_hash =
        "0e2c5c04c391ca0b8ca5fd9f6707bcddd53e8b7245c59331590d1c5490ffab7d505db"
        "0ba9b70a0f48e0f26ab6afeb84f600a7501a5fb1958f82f8623a7a1f692";

    EXPECT_EQ(d.hash(), expected_sha512_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, File)
{
    int fd = open("test.txt", O_RDONLY);
    const Digest d = DigestGenerator().hash(fd);
    EXPECT_EQ(
        d.hash(),
        "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
    EXPECT_EQ(d.size_bytes(), 4);
    close(fd);
}

TEST(DigestGeneratorTest, InvalidDigestFunction)
{
    ASSERT_EQ(DigestGenerator::supportedDigestFunctions().count(
                  DigestFunction::Value::DigestFunction_Value_UNKNOWN),
              0);

    ASSERT_THROW(const auto dg = DigestGenerator(
                     DigestFunction::Value::DigestFunction_Value_UNKNOWN),
                 std::runtime_error);
}

TEST(DigestGeneratorTest, VSONotImplemented)
{
    ASSERT_EQ(DigestGenerator::supportedDigestFunctions().count(
                  DigestFunction::Value::DigestFunction_Value_VSO),
              0);

    ASSERT_THROW(const auto dg = DigestGenerator(
                     DigestFunction::Value::DigestFunction_Value_VSO),
                 std::runtime_error);
}

class DigestGeneratorFixture : public ::testing::Test {
  protected:
    DigestGeneratorFixture()
    {
        // We generate a big string and write it to a file. It will be read in
        // multiple chunks.
        const size_t file_size = 10 * 1024 * 1024;
        d_data.reserve(file_size);

        while (d_data.size() + TEST_STRING.size() <= file_size) {
            d_data.append(TEST_STRING);
        }

        std::ofstream f(d_file.name(), std::ofstream::binary);
        f << d_data;
        f.close();

        d_fd = open(d_file.name(), O_RDONLY);
        assert(d_fd != -1);
    }

    buildboxcommon::TemporaryFile d_file;
    std::string d_data;
    int d_fd;

    void assert_digest_is_correct(DigestFunction_Value digest_function) const
    {
        const DigestGenerator dg(digest_function);
        const Digest from_file = dg.hash(d_fd);
        const Digest from_string = dg.hash(d_data);

        ASSERT_EQ(from_file, from_string);
    }

    ~DigestGeneratorFixture() { close(d_fd); }
};

TEST_F(DigestGeneratorFixture, DigestFromFileMD5)
{
    assert_digest_is_correct(DigestFunction_Value_MD5);
}

TEST_F(DigestGeneratorFixture, DigestFromFileSHA1)
{
    assert_digest_is_correct(DigestFunction_Value_SHA1);
}

TEST_F(DigestGeneratorFixture, DigestFromFileSHA256)
{
    assert_digest_is_correct(DigestFunction_Value_SHA256);
}

TEST_F(DigestGeneratorFixture, DigestFromFileSHA384)
{
    assert_digest_is_correct(DigestFunction_Value_SHA384);
}

TEST_F(DigestGeneratorFixture, DigestFromFileSHA512)
{
    assert_digest_is_correct(DigestFunction_Value_SHA512);
}
