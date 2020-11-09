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
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporarydirectory.h>
#include <buildboxcommon_temporaryfile.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(ProtosHeaderTest, DigestComparisonEqual)
{
    Digest d1, d2;

    d1.set_hash("hash");
    d1.set_size_bytes(1024);

    d2.CopyFrom(d1);

    ASSERT_TRUE(d1 == d2);
    ASSERT_FALSE(d1 != d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferentHash)
{
    Digest d1, d2;

    d1.set_hash("hash1");
    d2.set_hash("hash2");

    d1.set_size_bytes(1024);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
    ASSERT_TRUE(d1 != d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferentSize)
{
    Digest d1, d2;

    d1.set_hash("hash");
    d2.set_hash("hash");

    d1.set_size_bytes(100);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
    ASSERT_TRUE(d1 != d2);
}

TEST(ProtosHeaderTest, DigestComparisonDifferent)
{
    Digest d1, d2;

    d1.set_hash("hash1");
    d2.set_hash("hash2");

    d1.set_size_bytes(100);
    d2.set_size_bytes(1024);

    ASSERT_FALSE(d1 == d2);
    ASSERT_TRUE(d1 != d2);
}

TEST(ProtosHeaderTest, DigestComparisonLt)
{
    Digest d1, d2, d3;
    d1.set_hash("hash1");
    d1.set_size_bytes(10);

    d2.set_hash("hash1");
    d2.set_size_bytes(20);

    d3.set_hash("hash2");
    d3.set_size_bytes(1);

    ASSERT_LT(d1, d2);
    ASSERT_LT(d2, d3);
    ASSERT_LT(d1, d3);

    ASSERT_FALSE(d3 < d1);
    ASSERT_FALSE(d3 < d2);
    ASSERT_FALSE(d2 < d1);
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

TEST(ProtosUtilsTest, WriteProtoToFile)
{
    const Digest digest =
        CASHash::hash("We'll write the digest of this data to a file");

    TemporaryDirectory outputDirectory;
    const auto outputFile = buildboxcommon::FileUtils::joinPathSegments(
        outputDirectory.name(), "proto.out");

    ASSERT_NO_THROW(ProtoUtils::writeProtobufToFile(digest, outputFile));
    ASSERT_TRUE(buildboxcommon::FileUtils::isRegularFile(outputFile.c_str()));

    std::string fileContents;
    ASSERT_NO_THROW(fileContents =
                        FileUtils::getFileContents(outputFile.c_str()));

    Digest readDigest;
    ASSERT_TRUE(readDigest.ParseFromString(fileContents));
    ASSERT_EQ(readDigest, digest);
}

TEST(ProtosUtilsTest, WriteProtoToFileOverwritesContents)
{
    TemporaryFile outputFile;

    const Digest digestA = CASHash::hash("DigestA");
    ProtoUtils::writeProtobufToFile(digestA, outputFile.name());

    const Digest digestB = CASHash::hash("DigestB");
    ProtoUtils::writeProtobufToFile(digestB, outputFile.name());

    std::string fileContents;
    ASSERT_NO_THROW(fileContents =
                        FileUtils::getFileContents(outputFile.name()));

    Digest readDigest;
    ASSERT_TRUE(readDigest.ParseFromString(fileContents));
    ASSERT_EQ(readDigest, digestB);
}

TEST(ProtosUtilsTest, WriteProtoToFileThrowsOnError)
{
    google::rpc::Status statusProto;
    statusProto.set_message("Attempting to write to a directory will fail.");

    TemporaryDirectory directory;
    ASSERT_THROW(
        ProtoUtils::writeProtobufToFile(statusProto, directory.name()),
        std::system_error);
}

TEST(ProtosUtilsTest, ReadProtoFromFile)
{
    TemporaryFile outputFile;

    const Digest writtenDigest = CASHash::hash("Hash123");
    ASSERT_NO_THROW(
        ProtoUtils::writeProtobufToFile(writtenDigest, outputFile.name()));

    Digest readDigest;
    ASSERT_NO_THROW(readDigest = ProtoUtils::readProtobufFromFile<Digest>(
                        outputFile.name()));

    ASSERT_EQ(readDigest, writtenDigest);
}

TEST(ProtosUtilsTest, ReadProtoFromNonExistentPathThrows)
{
    const auto nonExistentPath = "/file/does/not/exist";
    ASSERT_FALSE(FileUtils::isRegularFile(nonExistentPath));

    EXPECT_THROW(ProtoUtils::readProtobufFromFile<Digest>(nonExistentPath),
                 std::runtime_error);
}

TEST(ProtosUtilsTest, ReadProtoFromMismatchedTypeThrows)
{
    TemporaryFile outputFile;
    ASSERT_NO_THROW(ProtoUtils::writeProtobufToFile(CASHash::hash("ABC"),
                                                    outputFile.name()));
    ASSERT_THROW(ProtoUtils::readProtobufFromFile<Action>(outputFile.name()),
                 std::runtime_error);
}
