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

#include <buildboxcommon_client.h>
#include <buildboxcommon_temporaryfile.h>

#include <gtest/gtest.h>

#include <build/bazel/remote/execution/v2/remote_execution_mock.grpc.pb.h>
#include <google/bytestream/bytestream_mock.grpc.pb.h>
#include <grpcpp/test/mock_stream.h>

#include <fstream>

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
    std::shared_ptr<MockCapabilitiesStub> capabilitiesClient;

    StubsFixture()
    {
        bytestreamClient =
            std::make_shared<google::bytestream::MockByteStreamStub>();
        casClient = std::make_shared<MockContentAddressableStorageStub>();
        capabilitiesClient = std::make_shared<MockCapabilitiesStub>();
    }
};

TEST_F(StubsFixture, InitTest)
{
    Client client;
    CacheCapabilities *cacheCapabilities = new CacheCapabilities();
    cacheCapabilities->set_max_batch_total_size_bytes(64);
    ServerCapabilities serverCapabilities;
    serverCapabilities.set_allocated_cache_capabilities(cacheCapabilities);

    EXPECT_CALL(*capabilitiesClient, GetCapabilities(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(serverCapabilities),
                        Return(grpc::Status::OK)));
    client.init(bytestreamClient, casClient, capabilitiesClient);
}

TEST_F(StubsFixture, InitCapabilitiesDidntReturnOk)
{
    /**
     * This test ensures that we're still ok if the capabilities endpoint
     * doesn't support the capabilities request or fails in some other way
     * (we just use a default value).
     */
    Client client;
    EXPECT_CALL(*capabilitiesClient, GetCapabilities(_, _, _))
        .WillOnce(Return(
            grpc::Status(grpc::UNIMPLEMENTED, "method not found for test")));
    client.init(bytestreamClient, casClient, capabilitiesClient);
}

class ClientTestFixture : public StubsFixture {
    /**
     * Fixture that provides a pre-instantiated client, as well as several
     * objects to be passed as arguments and returned from mocks.
     *
     * Inherits from the fixture that provides stubs.
     */
  protected:
    Client client;
    std::string content = "password";
    Digest digest;
    TemporaryFile tmpfile;
    google::bytestream::ReadResponse readResponse;
    grpc::testing::MockClientReader<google::bytestream::ReadResponse> *reader =
        new grpc::testing::MockClientReader<
            google::bytestream::ReadResponse>();
    grpc::testing::MockClientWriter<google::bytestream::WriteRequest> *writer =
        new grpc::testing::MockClientWriter<
            google::bytestream::WriteRequest>();

    ClientTestFixture()
        : client(bytestreamClient, casClient, capabilitiesClient,
                 MAX_BATCH_SIZE_BYTES)
    {
    }
};

TEST_F(ClientTestFixture, FetchStringTest)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_EQ(client.fetchString(digest), readResponse.data());
}

TEST_F(ClientTestFixture, FetchStringEmptyResponse)
{
    readResponse.set_data("");
    digest.set_size_bytes(0);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_EQ(client.fetchString(digest), readResponse.data());
}

TEST_F(ClientTestFixture, FetchStringSizeMismatch)
{
    readResponse.set_data(content);
    digest.set_size_bytes(99999);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_THROW(client.fetchString(digest), std::runtime_error);
}

TEST_F(ClientTestFixture, DownloadTest)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    client.download(tmpfile.fd(), digest);

    tmpfile.close();

    std::ifstream in(tmpfile.name());
    std::stringstream buffer;
    buffer << in.rdbuf();

    EXPECT_EQ(buffer.str(), readResponse.data());
}

TEST_F(ClientTestFixture, DownloadFdNotWritable)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)));

    EXPECT_THROW(client.download(-1, digest), std::system_error);
}

TEST_F(ClientTestFixture, DownloadSizeMismatch)
{
    readResponse.set_data(content);
    digest.set_size_bytes(99999999);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_THROW(client.download(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(ClientTestFixture, UploadStringTest)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(content, digest);
}

TEST_F(ClientTestFixture, UploadLargeStringTest)
{
    int contentLength = 3 * 1024 * 1024;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(content, digest);
}

TEST_F(ClientTestFixture, UploadExactStringTest)
{
    int contentLength = 1024 * 1024;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(content, digest);
}

TEST_F(ClientTestFixture, UploadJustLargerThanExactStringTest)
{
    int contentLength = 1024 * 1024 + 1;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(content, digest);
}

TEST_F(ClientTestFixture, UploadJustSmallerThanExactStringTest)
{
    int contentLength = 1024 * 1024 - 1;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(content, digest);
}

TEST_F(ClientTestFixture, UploadStringSizeMismatch)
{
    digest.set_size_bytes(9999999999999999);
    digest.set_hash("fakehash");

    EXPECT_THROW(client.upload(content, digest), std::logic_error);
}

TEST_F(ClientTestFixture, UploadStringWriteFailure)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(false));

    EXPECT_THROW(client.upload(content, digest), std::runtime_error);
}

TEST_F(ClientTestFixture, UploadStringDidntReturnOk)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::FAILED_PRECONDITION, "failing for test")));

    EXPECT_THROW(client.upload(content, digest), std::runtime_error);
}

TEST_F(ClientTestFixture, FileTooLargeToBatchUpload)
{
    const auto data = std::string(3 * MAX_BATCH_SIZE_BYTES, '_');

    Digest digest;
    digest.set_hash("dataHash");
    digest.set_size_bytes(data.size());

    Client::UploadRequest request(digest, data);
    std::vector<Client::UploadRequest> requests = {request};

    // Expecting it to fall back to a bytestream Write():
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));
    client.uploadBlobs(requests);
}

TEST_F(ClientTestFixture, FileTooLargeToBatchDownload)
{
    // Expecting it to fall back to a bytestream Read():
    const auto data = std::string(2 * MAX_BATCH_SIZE_BYTES, '-');

    Digest digest;
    digest.set_hash("dataHash");
    digest.set_size_bytes(data.size());

    const std::vector<Digest> requests = {digest};

    readResponse.set_data(data);
    digest.set_size_bytes(data.size());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    const auto downloads = client.downloadBlobs(requests);
    ASSERT_EQ(downloads.count(digest.hash()), 1);
    ASSERT_EQ(downloads.at(digest.hash()), data);
}

TEST_F(ClientTestFixture, DownloadBlobs)
{
    const std::vector<std::string> payload = {
        "a", "b", std::string(3 * MAX_BATCH_SIZE_BYTES, 'x'), "c"};
    const std::vector<std::string> hashes = {"hash0", "hash1", "hash2",
                                             "hash3"};

    // Creating list of requests...
    std::vector<Digest> requests;
    for (unsigned i = 0; i < payload.size(); i++) {
        Digest digest;
        digest.set_hash(hashes[i]);
        digest.set_size_bytes(payload[i].size());
        requests.push_back(digest);
    }
    ASSERT_EQ(requests.size(), payload.size());

    // We expect digests {0, 1, 3} to be requested with BatchReadBlobs().
    BatchReadBlobsResponse response;
    for (unsigned i = 0; i < payload.size(); i++) {
        if (i == 2)
            continue;
        auto entry = response.add_responses();
        entry->set_data(payload[i]);
        entry->mutable_digest()->CopyFrom(requests[i]);
    }
    EXPECT_CALL(*casClient.get(), BatchReadBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

    // And digest in index 2 with the Bytestream API:
    readResponse.set_data(payload[2]);
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    const auto downloads = client.downloadBlobs(requests);

    // We get all the data, and it's correct for each requested digest:
    ASSERT_EQ(downloads.size(), requests.size());
    ASSERT_EQ(downloads.at(hashes[0]), payload[0]);
    ASSERT_EQ(downloads.at(hashes[1]), payload[1]);
    ASSERT_EQ(downloads.at(hashes[2]), payload[2]);
    ASSERT_EQ(downloads.at(hashes[3]), payload[3]);
}

TEST_F(ClientTestFixture, DownloadBlobsFails)
{
    const std::vector<std::string> payload = {
        "a", std::string(3 * MAX_BATCH_SIZE_BYTES, 'x')};

    const std::vector<std::string> hashes = {"hash0", "hash1"};

    // Creating list of requests...
    std::vector<Digest> requests;
    for (unsigned i = 0; i < payload.size(); i++) {
        Digest digest;
        digest.set_hash(hashes[i]);
        digest.set_size_bytes(payload[i].size());
        requests.push_back(digest);
    }
    ASSERT_EQ(requests.size(), payload.size());

    // Both requests will fail with:
    const auto errorStatus =
        grpc::Status(grpc::StatusCode::NOT_FOUND, "Digest not found in CAS.");

    // We expect digests {0, 1, 3} to be requested with BatchReadBlobs().
    BatchReadBlobsResponse response;
    auto entry = response.add_responses();
    entry->mutable_digest()->CopyFrom(requests[0]);
    entry->mutable_status()->set_code(errorStatus.error_code());
    entry->mutable_status()->set_message(errorStatus.error_message());

    EXPECT_CALL(*casClient.get(), BatchReadBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(errorStatus)));

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    const auto downloads = client.downloadBlobs(requests);

    ASSERT_TRUE(downloads.empty());
}

TEST_F(ClientTestFixture, UploadBlobs)
{
    const std::vector<std::string> payload = {
        "a", "b", std::string(2 * MAX_BATCH_SIZE_BYTES, 'x'), "c"};
    const std::vector<std::string> hashes = {"hash0", "hash1", "hash2",
                                             "hash3"};

    // Creating list of requests...
    std::vector<Client::UploadRequest> requests;
    for (unsigned i = 0; i < payload.size(); i++) {
        Digest digest;
        digest.set_hash(hashes[i]);
        digest.set_size_bytes(payload[i].size());

        Client::UploadRequest request(digest, payload[i]);

        requests.push_back(request);
    }
    ASSERT_EQ(requests.size(), payload.size());

    // We expect digests {0, 1, 3} to be uploaded with BatchUpdateBlobs().
    BatchUpdateBlobsResponse response;
    for (unsigned i = 0; i < payload.size(); i++) {
        if (i == 2)
            continue;
        auto entry = response.add_responses();
        entry->mutable_digest()->CopyFrom(requests[i].digest);
    }
    EXPECT_CALL(*casClient.get(), BatchUpdateBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));
    // And digest in index 2 with the Bytestream API:
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    const auto failed_uploads = client.uploadBlobs(requests);
    ASSERT_TRUE(failed_uploads.empty());
}

TEST_F(ClientTestFixture, UploadBlobsReturnsFailures)
{
    const std::vector<std::string> payload = {
        "a", std::string(2 * MAX_BATCH_SIZE_BYTES, 'x')};
    const std::vector<std::string> hashes = {"hash0", "hash1"};

    // Creating list of requests...
    std::vector<Client::UploadRequest> requests;
    for (unsigned i = 0; i < payload.size(); i++) {
        Digest digest;
        digest.set_hash(hashes[i]);
        digest.set_size_bytes(payload[i].size());

        Client::UploadRequest request(digest, payload[i]);

        requests.push_back(request);
    }
    ASSERT_EQ(requests.size(), payload.size());

    // Both requests will fail with:
    const auto errorStatus = grpc::Status(grpc::StatusCode::INTERNAL,
                                          "Could not write data in CAS.");

    BatchUpdateBlobsResponse response;
    auto entry = response.add_responses();
    entry->mutable_digest()->CopyFrom(requests[0].digest);

    entry->mutable_status()->set_code(errorStatus.error_code());
    entry->mutable_status()->set_message(errorStatus.error_message());
    EXPECT_CALL(*casClient.get(), BatchUpdateBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));
    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(errorStatus));

    const auto failed_uploads = client.uploadBlobs(requests);

    ASSERT_EQ(failed_uploads.size(), 2);
    ASSERT_EQ(
        std::count(hashes.cbegin(), hashes.cend(), failed_uploads[0].hash()),
        1);
    ASSERT_EQ(
        std::count(hashes.cbegin(), hashes.cend(), failed_uploads[1].hash()),
        1);
}

class UploadFileFixture : public ClientTestFixture {
    /**
     * Instantiates a tempfile with some data for use in upload tests.
     * Inherits from the pre-instantiated client fixture.
     */
  protected:
    UploadFileFixture()
    {
        write(tmpfile.fd(), content.c_str(), content.length());
    }
};

TEST_F(UploadFileFixture, UploadFileTest)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadLargeFileTest)
{
    int contentLength = 3 * 1024 * 1024;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadExactFileTest)
{
    int contentLength = 1024 * 1024;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadJustLargerThanExactFileTest)
{
    int contentLength = 1024 * 1024 + 1;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadJustSmallerThanExactFileTest)
{
    int contentLength = 1024 * 1024 - 1;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    client.upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadFileReadFailure)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_THROW(client.upload(-40, digest), std::system_error);
}

TEST_F(UploadFileFixture, UploadFileWriteFailure)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(false));

    EXPECT_THROW(client.upload(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(UploadFileFixture, UploadFileDidntReturnOk)
{
    digest.set_size_bytes(content.length());
    digest.set_hash("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::FAILED_PRECONDITION, "failing for test")));

    EXPECT_THROW(client.upload(tmpfile.fd(), digest), std::runtime_error);
}
