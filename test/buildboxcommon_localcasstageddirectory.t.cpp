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
        response.set_path("/path/to/staged_dir/");
        EXPECT_CALL(*reader_writer, Read(_))
            .WillOnce(DoAll(SetArgPointee<0>(response), Return(true)));

        Digest digest;
        digest.set_hash("has12345");
        digest.set_size_bytes(1024);

        return std::make_unique<LocalCasStagedDirectory>(digest, path, client);
    }
};

TEST_F(LocalCasStagedDirectoryFixture, StageDirectory) { stageDirectory(""); }

// Just make sure constructor will accept non-empty strings
TEST_F(LocalCasStagedDirectoryFixture, StageDirectoryCustomPath)
{
    stageDirectory("/stage/here/");
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
