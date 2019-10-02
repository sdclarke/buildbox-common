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

#include <buildboxcommon_fallbackstageddirectory.h>

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
#include <unistd.h>

using namespace buildboxcommon;
using namespace build::bazel::remote::execution::v2;
using namespace testing;

const int64_t MAX_BATCH_SIZE_BYTES = 64;

class StubsFixture : public ::testing::Test {
    /**
     * Fixture that provides mock grpc stubs for instantiating the CAS
     * clients in this file's tests.
     */
  protected:
    std::shared_ptr<google::bytestream::MockByteStreamStub> bytestreamClient;
    std::shared_ptr<MockContentAddressableStorageStub> casClient;
    std::shared_ptr<MockLocalContentAddressableStorageStub> localCasClient;
    std::shared_ptr<MockCapabilitiesStub> capabilitiesClient;

    StubsFixture()
    {
        bytestreamClient =
            std::make_shared<google::bytestream::MockByteStreamStub>();
        casClient = std::make_shared<MockContentAddressableStorageStub>();
        localCasClient =
            std::make_shared<MockLocalContentAddressableStorageStub>();
        capabilitiesClient = std::make_shared<MockCapabilitiesStub>();
    }
};

class CaptureTestFixture : public StubsFixture {
    /**
     * Fixture that provides a pre-instantiated client, as well as several
     * objects to be passed as arguments and returned from mocks.
     *
     * Inherits from the fixture that provides stubs.
     */
  protected:
    std::string content = "password";
    Digest digest;
    TemporaryFile tmpfile;
    google::bytestream::ReadResponse readResponse;
    grpc::testing::MockClientReader<google::bytestream::ReadResponse> *reader =
        new grpc::testing::MockClientReader<
            google::bytestream::ReadResponse>();
    std::shared_ptr<Client> client;

    CaptureTestFixture()
    {
        client = std::make_shared<Client>(bytestreamClient, casClient,
                                          localCasClient, capabilitiesClient,
                                          MAX_BATCH_SIZE_BYTES);
    }
};

/*
 * Construct a new writer for every write raw request
 * Also store argument for testing
 */
auto getWriter(std::vector<WriteRequest>::iterator &iter)
{
    grpc::testing::MockClientWriter<google::bytestream::WriteRequest> *writer =
        new grpc::testing::MockClientWriter<
            google::bytestream::WriteRequest>();
    EXPECT_CALL(*writer, Write(_, _))
        .WillRepeatedly(DoAll(SaveArg<0>(iter++), Return(true)));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));
    return writer;
}

/*
 * Copy source directory  into destination path
 * Destination should already exist
 */
void copyDirectory(std::string source, std::string destination)
{
    std::ostringstream copyCommand;
    copyCommand << "cp -r " << source << " " << destination;
    system(copyCommand.str().c_str());
}

TEST_F(CaptureTestFixture, CaptureDirectoryTest)
{
    std::vector<WriteRequest> requests(4);
    auto iter = requests.begin();

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Using WillRepeatedly will break the test
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(Return(getWriter(iter)))
        .WillOnce(Return(getWriter(iter)))
        .WillOnce(Return(getWriter(iter)))
        .WillOnce(Return(getWriter(iter)));

    FallbackStagedDirectory fs(digest, client);

    /*
     * upload_test
     * ├── test.txt
     * └── child
     * 	   └── child_test.txt
     */
    copyDirectory("upload_test", fs.getPath() + std::string("/upload_testx"));
    OutputDirectory output_dir = fs.captureDirectory("upload_testx");

    // expected filecontent hashes of files and directories
    // top level tree digest is expected to be in here
    std::vector<std::string> expected_hashes = {
        "4620034124b155a66572b8f5e6205d17cfc11de6cdabb4406e97cbf5d4e5fb9f",
        "54f2540e9e3c7681005e969e9f0efec0912e2a7807a537d46eedbb805479b9f2",
        "70986761e554610afe6ecf9e81e789c64ac954ad24f8dca87623220a8c361995",
        "aa7d7a98cbe729f06c50f0a67d3afa8de2d7f68196e89521c10fbf65b1768db0"};

    std::string prefix = "uploads//blobs/";
    std::string tree_digest = output_dir.mutable_tree_digest()->hash();

    EXPECT_EQ(
        tree_digest,
        "aa7d7a98cbe729f06c50f0a67d3afa8de2d7f68196e89521c10fbf65b1768db0");
    EXPECT_EQ(requests.size(), 4);

    auto compare = [](auto a, auto b) {
        return a.resource_name() < b.resource_name();
    };
    std::sort(requests.begin(), requests.end(), compare);
    for (int i = 0; i < requests.size(); i++) {
        // Check if expected hashes are in response
        std::string response = requests[i].resource_name();
        EXPECT_EQ(response.find(prefix + expected_hashes[i]), 0);
    }

    // The directory is staged. Let's now simulate capturing outputs:
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
    fs.captureAllOutputs(command, &action_result, capture_file_function,
                         capture_directory_function);

    ASSERT_EQ(captured_files.size(), 2);
    ASSERT_EQ(captured_files.count("/a.out"), 1);
    ASSERT_EQ(captured_files.count("/lib.so"), 1);

    ASSERT_EQ(captured_directories.size(), 1);
    ASSERT_EQ(captured_directories.count("/include"), 1);
}
