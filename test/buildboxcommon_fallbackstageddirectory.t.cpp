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
#include <buildboxcommon_timeutils.h>

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

TEST_P(CaptureTestFixtureParameter, CaptureDirectoryTest)
{
    // FallBackStagedDirectory will first download the contents using the CAS
    // client:
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Get the stage location:
    const std::string stage_location = GetParam();
    FallbackStagedDirectory fs(digest, stage_location, client);

    // Making sure `fs` staged the directory in correct location:
    const std::string staged_path = fs.getPath();
    EXPECT_EQ(staged_path.find(stage_location), 0);

    const Digest tree_digest = make_digest("directory-tree");
    // Verifying that the CAS client's `uploadDirectory()` method is invoked
    // with the correct absolute path:
    std::string upload_directory_argument;
    auto upload_directory_function = [&upload_directory_argument,
                                      tree_digest](const std::string &path) {
        upload_directory_argument = path;
        return tree_digest;
    };

    // We want to capture `upload_testx/` located in the `staged_path`.
    // We expect the CAS client to be invoked for `staged_path/upload_testx`.
    const std::string subdirectory_to_capture = "upload_testx";
    const std::string absolute_path_to_capture =
        staged_path + "/" + subdirectory_to_capture;
    buildboxcommon::FileUtils::createDirectory(
        absolute_path_to_capture.c_str());

    const OutputDirectory output_dir = fs.captureDirectory(
        subdirectory_to_capture.c_str(), upload_directory_function);

    ASSERT_EQ(upload_directory_argument, absolute_path_to_capture);

    // The OutputDirectory contains the correct information:
    ASSERT_EQ(output_dir.tree_digest(), tree_digest);
    ASSERT_EQ(output_dir.path(), "upload_testx");
}

TEST_F(CaptureTestFixtureParameter, CaptureDirectoryEscapingInputRoot)
{

    // FallBackStagedDirectory will first download the contents using the CAS
    // client:
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Setting up a directory structure with an escaping symlink:
    // top_level/  <------------|
    //    | input_root/         |
    //          | symlink ------|
    TemporaryDirectory top_level_directory;
    TemporaryDirectory input_root(top_level_directory.name(), "tmp-test");

    // This symlink goes above the input root, so it must not be followed when
    // capturing:
    const std::string symlink_path =
        std::string(input_root.name()) + "/escaping-symlink";
    ASSERT_EQ(symlink(top_level_directory.name(), symlink_path.c_str()), 0);

    // Stage:
    FallbackStagedDirectory fs(digest, input_root.name(), client);
    const std::string staged_path = fs.getPath();

    auto dummy_upload_directory_function = [](const std::string &) {
        return make_digest("dummy-tree-digest");
    };

    // And attempt to capture the symlink:
    const OutputDirectory output_dir =
        fs.captureDirectory("symlink/", dummy_upload_directory_function);

    ASSERT_TRUE(output_dir.path().empty());
    ASSERT_EQ(output_dir.tree_digest(), Digest());
}

TemporaryFile createExecutableTestFile(const std::string &dir_path,
                                       Digest *file_digest)
{
    TemporaryFile file(dir_path.c_str(), "test-file");
    std::ofstream ofstream(file.name(), std::ios::binary);

    ofstream << "Test contents...";
    ofstream.flush();

    file_digest->CopyFrom(CASHash::hashFile(file.name()));

    FileUtils::makeExecutable(file.name());

    return file;
}

TEST_F(CaptureTestFixtureParameter, CaptureFileTest)
{
    // FallBackStagedDirectory will first download the contents using the CAS
    // client:
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Get the stage location:
    TemporaryDirectory stage_directory;
    const std::string stage_location = stage_directory.name();
    FallbackStagedDirectory fs(digest, stage_location, client);

    const std::string staged_path = fs.getPath();
    EXPECT_EQ(staged_path.find(stage_location), 0);

    // Creating a file inside the staged directory that we'll capture:
    Digest staged_file_digest;
    TemporaryFile staged_file =
        createExecutableTestFile(staged_path, &staged_file_digest);

    const std::string staged_file_path = std::string(staged_file.name());
    const std::string staged_file_name =
        staged_file_path.substr(staged_file_path.rfind('/') + 1);

    // Capturing file without properties (default behavior):
    {
        Digest captured_digest;
        const auto dummy_upload_function =
            [&captured_digest](const int, const Digest &digest) {
                captured_digest = digest;
                return;
            };

        const OutputFile output_file_no_mtime =
            fs.captureFile(staged_file_name.c_str(), dummy_upload_function);
        ASSERT_EQ(captured_digest, staged_file_digest);

        ASSERT_EQ(output_file_no_mtime.path(), staged_file_name);
        ASSERT_EQ(output_file_no_mtime.digest(), staged_file_digest);
        ASSERT_TRUE(output_file_no_mtime.is_executable());
        ASSERT_EQ(output_file_no_mtime.node_properties().mtime().seconds(), 0);
        ASSERT_EQ(output_file_no_mtime.node_properties().mtime().nanos(), 0);
    }

    {
        Digest captured_digest;
        const auto dummy_upload_function =
            [&captured_digest](const int, const Digest &digest) {
                captured_digest = digest;
                return;
            };

        // Capturing the file and its mtime:
        const OutputFile output_file_with_mtime = fs.captureFile(
            staged_file_name.c_str(), dummy_upload_function, true);

        ASSERT_EQ(captured_digest, staged_file_digest);

        ASSERT_EQ(output_file_with_mtime.path(), staged_file_name);
        ASSERT_EQ(output_file_with_mtime.digest(), staged_file_digest);
        ASSERT_TRUE(output_file_with_mtime.is_executable());

        const auto expected_mtime = TimeUtils::make_timestamp(
            FileUtils::getFileMtime(staged_file_path.c_str()));
        ASSERT_EQ(output_file_with_mtime.node_properties().mtime(),
                  expected_mtime);
    }
}

TEST_F(CaptureTestFixtureParameter, CaptureFileEscapingInputRootTest)
{
    // FallBackStagedDirectory will first download the contents using the CAS
    // client:
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Setting up a directory structure with an escaping symlink:
    // top_level/  <------------|
    //    | input_root/         |
    //          | symlink ------|
    TemporaryDirectory top_level_directory;
    TemporaryDirectory input_root(top_level_directory.name(), "tmp-test");

    // This symlink goes above the input root, so it must not be followed when
    // capturing:
    const std::string symlink_path =
        std::string(input_root.name()) + "/escaping-symlink";
    ASSERT_EQ(symlink(top_level_directory.name(), symlink_path.c_str()), 0);

    // Stage:
    const std::string stage_location = input_root.name();
    FallbackStagedDirectory fs(digest, stage_location, client);
    const std::string staged_path = fs.getPath();

    const auto dummy_upload_function = [](const int, const Digest &) {
        return make_digest("dummy-digest");
    };

    // And attempt to capture:
    const OutputFile output_file =
        fs.captureFile("symlink", dummy_upload_function);

    ASSERT_TRUE(output_file.path().empty());
    ASSERT_EQ(output_file.digest(), Digest());
}

TEST_F(CaptureTestFixtureParameter, CaptureNonExistentFileDoesNotCallUpload)
{
    // FallBackStagedDirectory will first download the contents using the CAS
    // client:
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillRepeatedly(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillRepeatedly(Return(grpc::Status::OK));

    // Get the stage location:
    TemporaryDirectory stage_directory;
    const std::string stage_location = stage_directory.name();
    FallbackStagedDirectory fs(digest, stage_location, client);

    const std::string staged_path = fs.getPath();

    // We will try and capture a file that does not exist in the staged
    // directory.
    // The upload function will never be invoked and the returned
    // `OutputFile` will be empty.
    const std::string staged_file_path =
        staged_path + "/non-existent-file.txt";

    bool upload_called = false;
    const auto dummy_upload_function = [&upload_called](const int,
                                                        const Digest &) {
        upload_called = true;
        return;
    };
    const OutputFile output_file =
        fs.captureFile(staged_file_path.c_str(), dummy_upload_function);

    ASSERT_FALSE(upload_called);

    ASSERT_TRUE(output_file.path().empty());
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

/*
 * Fallbackstageddirectory will have different behaviour
 * depending on the stage location. Test both empty string
 * and current working directory, to check both these cases
 */
INSTANTIATE_TEST_CASE_P(CaptureTests, CaptureTestFixtureParameter,
                        ::testing::Values("",
                                          get_current_working_directory()));
