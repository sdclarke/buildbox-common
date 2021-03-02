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

#include <buildboxcommon_localcasstageddirectory.h>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_client.h>
#include <buildboxcommon_protos.h>
#include <buildboxcommon_temporarydirectory.h>
#include <buildboxcommon_temporaryfile.h>

#include <gtest/gtest.h>

#include <build/bazel/remote/execution/v2/remote_execution_mock.grpc.pb.h>
#include <build/buildgrid/local_cas_mock.grpc.pb.h>
#include <google/bytestream/bytestream_mock.grpc.pb.h>
#include <grpcpp/test/mock_stream.h>

#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace buildboxcommon;
using namespace testing;

const int64_t MAX_BATCH_SIZE_BYTES = 64;

class LocalCasStagedDirectoryFixture : public ::testing::Test {
    /**
     * Fixture that provides a pre-instantiated client, as well as several
     * objects to be passed as arguments and returned from mocks.
     *
     * Inherits from the fixture that provides stubs.
     */
  protected:
    LocalCasStagedDirectoryFixture()
    {
        bytestreamClient =
            std::make_shared<google::bytestream::MockByteStreamStub>();
        casClient = std::make_shared<MockContentAddressableStorageStub>();
        localCasClient =
            std::make_shared<MockLocalContentAddressableStorageStub>();
        capabilitiesClient = std::make_shared<MockCapabilitiesStub>();

        client = std::make_shared<Client>(bytestreamClient, casClient,
                                          localCasClient, capabilitiesClient,
                                          MAX_BATCH_SIZE_BYTES);
    }

    Digest digest;
    grpc::testing::MockClientReaderWriter<
        typename build::buildgrid::StageTreeRequest,
        typename build::buildgrid::StageTreeResponse> *reader_writer =
        new grpc::testing::MockClientReaderWriter<
            typename build::buildgrid::StageTreeRequest,
            typename build::buildgrid::StageTreeResponse>();
    std::shared_ptr<google::bytestream::MockByteStreamStub> bytestreamClient;
    std::shared_ptr<MockContentAddressableStorageStub> casClient;
    std::shared_ptr<MockLocalContentAddressableStorageStub> localCasClient;
    std::shared_ptr<MockCapabilitiesStub> capabilitiesClient;

    std::shared_ptr<Client> client;

    TemporaryDirectory staged_directory;

    std::unique_ptr<LocalCasStagedDirectory>
    stageDirectory(const std::string &path)
    {
        EXPECT_CALL(*localCasClient.get(), StageTreeRaw(_))
            .WillOnce(Return(reader_writer));

        // The client will issue 2 requests: the actual `StageTreeRequest` and
        // an empty message to indicate to the server that it can clean up.
        EXPECT_CALL(*reader_writer, Write(_, _))
            .Times(2)
            .WillRepeatedly(Return(true));

        EXPECT_CALL(*reader_writer, WritesDone());

        StageTreeResponse response;
        // Returning a valid directory:
        response.set_path(staged_directory.name());
        EXPECT_CALL(*reader_writer, Read(_))
            .WillOnce(DoAll(SetArgPointee<0>(response), Return(true)));

        Digest digest;
        digest.set_hash_other("has12345");
        digest.set_size_bytes(1024);

        return std::make_unique<LocalCasStagedDirectory>(digest, path, client);
    }
};

TEST_F(LocalCasStagedDirectoryFixture, StageDirectory) { stageDirectory(""); }

// Just make sure constructor will accept non-empty strings
TEST_F(LocalCasStagedDirectoryFixture, StageDirectoryCustomPath)
{
    stageDirectory(staged_directory.name());
}

TEST_F(LocalCasStagedDirectoryFixture, CaptureCommandOutputs)
{
    auto fs = stageDirectory("");

    // The directory is staged. Let's now capture the outputs:
    Command command;
    *command.add_output_files() = "a.out";
    *command.add_output_files() = "lib.so";

    *command.add_output_directories() = "include";

    std::multiset<std::string> captured_files, captured_directories;

    StagedDirectory::CaptureFileCallback capture_file_function =
        [&](const char *relative_path) {
            captured_files.insert(relative_path);
            return OutputFile();
        };

    StagedDirectory::CaptureDirectoryCallback capture_directory_function =
        [&](const char *relative_path) {
            captured_directories.insert(relative_path);
            return OutputDirectory();
        };

    ActionResult action_result;
    fs->captureAllOutputs(command, &action_result, capture_file_function,
                          capture_directory_function);

    ASSERT_EQ(captured_files.size(), 2);
    ASSERT_EQ(captured_files.count("a.out"), 1);
    ASSERT_EQ(captured_files.count("lib.so"), 1);

    ASSERT_EQ(captured_directories.size(), 1);
    ASSERT_EQ(captured_directories.count("include"), 1);
}

TEST_F(LocalCasStagedDirectoryFixture, CaptureNonExistentDirectory)
{
    Command command;
    auto fs = stageDirectory("");

    const auto non_existent_path = "/dir/that/does/not/exist";
    ASSERT_FALSE(FileUtils::isDirectory(non_existent_path));

    const auto captured_directory =
        fs->captureDirectory(non_existent_path, command);

    ASSERT_TRUE(captured_directory.path().empty());
}

TEST_F(LocalCasStagedDirectoryFixture, CaptureNonExistentFile)
{
    Command command;
    auto fs = stageDirectory("");

    const auto non_existent_path = "/file/that/does/not/exist";
    ASSERT_FALSE(FileUtils::isRegularFile(non_existent_path));

    const auto captured_file = fs->captureFile(non_existent_path, command);

    ASSERT_TRUE(captured_file.path().empty());
}

TEST_F(LocalCasStagedDirectoryFixture, CaptureFileWithEscapingSymlink)
{
    TemporaryDirectory top_level_directory;
    TemporaryDirectory stage_directory(top_level_directory.name(),
                                       "tmp-test-dir");

    // top_level_directory/
    //     | file  <---------------|
    //     | stage_directory/      x  <-- input root level
    //          | symlink ---------|

    Command command;
    auto fs = stageDirectory(stage_directory.name());

    // Creating a file in `top_level_directory` and a symlink to it that will
    // escape the input root:
    const auto symlink_destination =
        std::string(top_level_directory.name()) + "file";
    FileUtils::writeFileAtomically(symlink_destination.c_str(), "");

    const auto symlink_path = std::string(stage_directory.name()) + "/symlink";
    ASSERT_EQ(symlink(top_level_directory.name(), symlink_path.c_str()), 0);
    ASSERT_TRUE(FileUtils::isSymlink(symlink_path.c_str()));

    const auto captured_file = fs->captureFile(symlink_path.c_str(), command);

    ASSERT_TRUE(captured_file.path().empty());
}

TEST_F(LocalCasStagedDirectoryFixture, CaptureDirectoryWithEscapingSymlink)
{
    TemporaryDirectory top_level_directory;
    TemporaryDirectory stage_directory(top_level_directory.name(),
                                       "tmp-test-dir");

    // top_level_directory/  <-----|
    //     | stage_directory/      x  <-- input root level
    //          | symlink ---------|

    Command command;
    auto fs = stageDirectory(stage_directory.name());

    const auto symlink_path = std::string(stage_directory.name()) + "/symlink";
    ASSERT_EQ(symlink(top_level_directory.name(), symlink_path.c_str()), 0);
    ASSERT_TRUE(FileUtils::isSymlink(symlink_path.c_str()));

    const auto captured_directory =
        fs->captureDirectory(symlink_path.c_str(), command);

    ASSERT_TRUE(captured_directory.path().empty());
}
