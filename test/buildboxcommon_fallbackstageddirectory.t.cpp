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

class CaptureTestFixtureParameter
    : public CaptureTestFixture,
      public ::testing::WithParamInterface<std::string> {
    /**
     * Fixture needed to parameterize tests
     */
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

/*
 * Get current working directory.
 */
std::string get_current_working_directory()
{
    unsigned int bufferSize = 1024;
    while (true) {
        std::unique_ptr<char[]> buffer(new char[bufferSize]);
        char *cwd = getcwd(buffer.get(), bufferSize);

        if (cwd != nullptr) {
            return std::string(cwd);
        }
        else if (errno == ERANGE) {
            bufferSize *= 2;
        }
        else {
            throw std::runtime_error("current working directory not found");
        }
    }
}

std::set<std::string>
getHashesFromRequest(const std::vector<WriteRequest> &requests,
                     const std::string &resource_name_prefix)
{
    std::set<std::string> requested_hashes;

    for (const auto &entry : requests) {
        const std::string resource_name = entry.resource_name();
        // Resource names should have the form {prefix/hash/size_bytes},
        // e.g.: "uploads//blobs/fb7adfd80c02df9fa12fb82aabe167fe577a79b5/64"

        const auto hash_start = resource_name_prefix.size();
        const std::string resource_digest_string =
            resource_name.substr(hash_start);

        // Getting only the hash portion, dropping the size:
        const std::string resource_hash = resource_digest_string.substr(
            0, resource_digest_string.rfind('/'));

        requested_hashes.insert(resource_hash);
    }

    return requested_hashes;
}

TEST_P(CaptureTestFixtureParameter, CaptureDirectoryTest)
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

    // Get the stage location.
    const std::string stage_location = GetParam();
    FallbackStagedDirectory fs(digest, stage_location, client);

    // Make sure fs staged the directory in correct location
    const std::string staged_path = fs.getPath();
    EXPECT_EQ(staged_path.find(stage_location), 0);

    EXPECT_EQ(requests.size(), 4);

    /*
     * upload_test
     * ├── test.txt
     * └── child
     * 	   └── child_test.txt
     */
    copyDirectory("upload_test", fs.getPath() + std::string("/upload_testx"));
    OutputDirectory output_dir = fs.captureDirectory("upload_testx");

    const std::string prefix = "uploads//blobs/";
    const Digest tree_digest = output_dir.tree_digest();

    // Expected hashes of the files and root directory tree digest:
    std::vector<Digest> expected_digests = {
        CASHash::hashFile(fs.getPath() +
                          std::string("/upload_testx/test.txt")),
        CASHash::hashFile(fs.getPath() +
                          std::string("/upload_testx/child/child_test.txt")),
        tree_digest};

    // Taking the hashes from the request and checking that the files to be
    // captured are there:
    const std::set<std::string> requested_hashes =
        getHashesFromRequest(requests, prefix);
    for (const Digest &digest : expected_digests) {
        ASSERT_EQ(requested_hashes.count(digest.hash()), 1);
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

/*
 * Fallbackstageddirectory will have different behaviour
 * depending on the stage location. Test both empty string
 * and current working directory, to check both these cases
 */
INSTANTIATE_TEST_CASE_P(CaptureTests, CaptureTestFixtureParameter,
                        ::testing::Values("",
                                          get_current_working_directory()));
