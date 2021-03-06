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

#include "buildboxcommontest_utils.h"
#include <buildboxcommon_client.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_merklize.h>
#include <buildboxcommon_temporarydirectory.h>
#include <buildboxcommon_temporaryfile.h>
#include <buildboxcommon_timeutils.h>
#include <gtest/gtest.h>

#include <build/bazel/remote/execution/v2/remote_execution_mock.grpc.pb.h>
#include <build/buildgrid/local_cas_mock.grpc.pb.h>
#include <google/bytestream/bytestream_mock.grpc.pb.h>
#include <grpcpp/test/mock_stream.h>

#include <algorithm>
#include <fstream>

using namespace buildboxcommon;
using namespace testing;

const int64_t MAX_BATCH_SIZE_BYTES = 2 * 1024;

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
    client.init(bytestreamClient, casClient, localCasClient,
                capabilitiesClient);

    ASSERT_TRUE(client.instanceName().empty());
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
    client.init(bytestreamClient, casClient, localCasClient,
                capabilitiesClient);
}

class ClientTestFixture : public StubsFixture, public Client {
    /**
     * Fixture that provides a pre-instantiated client, as well as several
     * objects to be passed as arguments and returned from mocks.
     *
     * Inherits from the fixture that provides stubs.
     */
  protected:
    const std::string client_instance_name = "CasTestInstance123";
    const std::string content = "password";
    Digest digest;
    TemporaryFile tmpfile;
    google::bytestream::ReadResponse readResponse;
    google::bytestream::WriteResponse writeResponse;

    grpc::testing::MockClientReader<google::bytestream::ReadResponse> *reader =
        new grpc::testing::MockClientReader<
            google::bytestream::ReadResponse>();

    grpc::testing::MockClientWriter<google::bytestream::WriteRequest> *writer =
        new grpc::testing::MockClientWriter<
            google::bytestream::WriteRequest>();

    grpc::testing::MockClientReaderWriter<
        typename build::buildgrid::StageTreeRequest,
        typename build::buildgrid::StageTreeResponse> *reader_writer =
        new grpc::testing::MockClientReaderWriter<
            typename build::buildgrid::StageTreeRequest,
            typename build::buildgrid::StageTreeResponse>();

    grpc::testing::MockClientReader<
        typename build::bazel::remote::execution::v2::GetTreeResponse>
        *gettreereader = new grpc::testing::MockClientReader<
            typename build::bazel::remote::execution::v2::GetTreeResponse>();

    ClientTestFixture(int64_t max_batch_size_bytes = MAX_BATCH_SIZE_BYTES)
        : Client(bytestreamClient, casClient, localCasClient,
                 capabilitiesClient, max_batch_size_bytes)
    {
        this->setInstanceName(client_instance_name);
        this->d_grpcRetryLimit = 1;
        this->d_grpcRetryDelay = 1;
    }
};

TEST_F(ClientTestFixture, FetchStringTest)
{
    readResponse.set_data(content);
    digest = CASHash::hash(content);

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    ReadRequest request;
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillOnce(DoAll(SaveArg<1>(&request), Return(reader)));

    // Setting a new instance name with the client's setter:
    const std::string instance_name = "newInstanceName!";
    this->setInstanceName(instance_name);
    ASSERT_EQ(this->instanceName(), instance_name);

    EXPECT_EQ(this->fetchString(digest), readResponse.data());
    EXPECT_EQ(request.resource_name().substr(0, instance_name.size()),
              instance_name);
}

TEST_F(ClientTestFixture, FetchStringEmptyResponse)
{
    readResponse.set_data("");
    digest = CASHash::hash("");

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    EXPECT_EQ(this->fetchString(digest), readResponse.data());
}

TEST_F(ClientTestFixture, FetchStringSizeMismatch)
{
    readResponse.set_data(content);
    digest.set_size_bytes(99999);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish())
        .WillOnce(
            Return(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "")));

    EXPECT_THROW(this->fetchString(digest), std::runtime_error);
}

TEST_F(ClientTestFixture, FetchStringServerError)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::StatusCode::NOT_FOUND, "Digest not found!")));

    EXPECT_THROW(this->fetchString(digest), std::runtime_error);
}

TEST_F(ClientTestFixture, FetchStringServerRetryableError)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    ASSERT_EQ(d_grpcRetryLimit, 1);

    auto bytestreamReader1 = new grpc::testing::MockClientReader<
        google::bytestream::ReadResponse>();
    auto bytestreamReader2 = new grpc::testing::MockClientReader<
        google::bytestream::ReadResponse>();

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillOnce(Return(bytestreamReader1))
        .WillOnce(Return(bytestreamReader2));

    EXPECT_CALL(*bytestreamReader1, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*bytestreamReader1, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                      "Something is wrong right now.")));

    EXPECT_CALL(*bytestreamReader2, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*bytestreamReader2, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                      "Something is still wrong.")));

    ASSERT_THROW(this->fetchString(digest), GrpcError);
}

TEST_F(ClientTestFixture, FindMissingBlobsSuccessful)
{
    Digest missingDigest;
    missingDigest.set_hash_other("missing-hash");

    FindMissingBlobsResponse response;
    response.add_missing_blob_digests()->CopyFrom(missingDigest);

    Digest presentDigest;
    presentDigest.set_hash_other("present-hash");

    const std::vector<Digest> findMissingList = {missingDigest, presentDigest};

    EXPECT_CALL(*casClient.get(), FindMissingBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

    const auto missingBlobsResponse = findMissingBlobs(findMissingList);
    ASSERT_EQ(missingBlobsResponse.size(), 1);
    ASSERT_EQ(missingBlobsResponse.front(), missingDigest);
}

TEST_F(ClientTestFixture, FindMissingBlobsRetryableError)
{
    Digest missingDigest;
    missingDigest.set_hash_other("missing-hash");

    FindMissingBlobsResponse response;
    response.add_missing_blob_digests()->CopyFrom(missingDigest);

    Digest presentDigest;
    presentDigest.set_hash_other("present-hash");

    const std::vector<Digest> findMissingList = {missingDigest, presentDigest};

    const auto errorStatus = grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                          "Something is wrong right now.");
    EXPECT_CALL(*casClient.get(), FindMissingBlobs(_, _, _))
        .Times(d_grpcRetryLimit + 1)
        .WillRepeatedly(Return(errorStatus));

    try {
        findMissingBlobs(findMissingList);
    }
    catch (const GrpcError &e) {
        EXPECT_EQ(e.status.error_code(), errorStatus.error_code());
        EXPECT_EQ(e.status.error_message(), errorStatus.error_message());
    }
    catch (...) {
        FAIL() << "findMissingBlobs() threw Unexpected exception type.\n";
    }
}

TEST_F(ClientTestFixture, DownloadTest)
{
    readResponse.set_data(content);
    digest = CASHash::hash(content);

    ReadRequest request;
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillOnce(DoAll(SaveArg<1>(&request), Return(reader)));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    this->download(tmpfile.fd(), digest);

    tmpfile.close();

    std::ifstream in(tmpfile.name());
    std::stringstream buffer;
    buffer << in.rdbuf();

    EXPECT_EQ(buffer.str(), readResponse.data());
    EXPECT_EQ(request.resource_name().substr(0, client_instance_name.size()),
              client_instance_name);
}

TEST_F(ClientTestFixture, DownloadFdNotWritable)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)));

    EXPECT_THROW(this->download(-1, digest), std::system_error);
}

TEST_F(ClientTestFixture, DownloadSizeMismatch)
{
    readResponse.set_data(content);
    digest.set_size_bytes(99999999);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    EXPECT_THROW(this->download(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(ClientTestFixture, DownloadHashMismatch)
{
    readResponse.set_data(content);
    digest.set_hash_other("invalid-hash");
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    EXPECT_THROW(this->download(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(ClientTestFixture, DownloadServerError)
{
    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::NOT_FOUND,
                                      "Do not have the digest (fatal)")));
    EXPECT_THROW(this->download(tmpfile.fd(), digest), GrpcError);
}

TEST_F(ClientTestFixture, DownloadRetryableServerError)
{

    readResponse.set_data(content);
    digest.set_size_bytes(content.length());

    ASSERT_EQ(d_grpcRetryLimit, 1);

    auto bytestreamReader1 = new grpc::testing::MockClientReader<
        google::bytestream::ReadResponse>();
    auto bytestreamReader2 = new grpc::testing::MockClientReader<
        google::bytestream::ReadResponse>();

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _))
        .WillOnce(Return(bytestreamReader1))
        .WillOnce(Return(bytestreamReader2));

    EXPECT_CALL(*bytestreamReader1, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*bytestreamReader1, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                      "Something is wrong right now.")));

    EXPECT_CALL(*bytestreamReader2, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*bytestreamReader2, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                      "Something is still wrong.")));

    EXPECT_THROW(this->download(tmpfile.fd(), digest), GrpcError);
}

TEST_F(ClientTestFixture, UploadStringTest)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    WriteRequest request;
    EXPECT_CALL(*writer, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(content, digest);
    EXPECT_EQ(request.resource_name().substr(0, client_instance_name.size()),
              client_instance_name);
}

TEST_F(ClientTestFixture, UploadStringCommittedSizeMismatch)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes() - 1);
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    WriteRequest request;
    EXPECT_CALL(*writer, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    ASSERT_THROW(this->upload(content, digest), std::runtime_error);
}

TEST_F(ClientTestFixture, UploadLargeStringTest)
{
    int contentLength = 3 * 1024 * 1024;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(content, digest);
}

TEST_F(ClientTestFixture, UploadExactStringTest)
{
    int contentLength = 1024 * 1024;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(content, digest);
}

TEST_F(ClientTestFixture, UploadJustLargerThanExactStringTest)
{
    int contentLength = 1024 * 1024 + 1;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(content, digest);
}

TEST_F(ClientTestFixture, UploadJustSmallerThanExactStringTest)
{
    int contentLength = 1024 * 1024 - 1;
    std::string content = std::string(contentLength, 'f');
    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(content, digest);
}

TEST_F(ClientTestFixture, UploadStringSizeMismatch)
{
    digest.set_size_bytes(9999999999999999);
    digest.set_hash_other("fakehash");

    EXPECT_THROW(this->upload(content, digest), std::logic_error);
}

TEST_F(ClientTestFixture, UploadAlreadyExistingString)
{
    const std::string data("This blob is already present in the remote.");
    const Digest digest = CASHash::hash(data);

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(grpc::Status(grpc::Status::OK)));

    ASSERT_NO_THROW(this->upload(data, digest));
}

TEST_F(ClientTestFixture, UploadStringDidntReturnOk)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::FAILED_PRECONDITION, "failing for test")));

    EXPECT_THROW(this->upload(content, digest), GrpcError);
}

TEST_F(ClientTestFixture, UploadStringRetriableError)
{
    auto *writer1 = new grpc::testing::MockClientWriter<
        google::bytestream::WriteRequest>();

    auto *writer2 = new grpc::testing::MockClientWriter<
        google::bytestream::WriteRequest>();

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(Return(writer1))
        .WillOnce(Return(writer2));

    EXPECT_CALL(*writer1, Write(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer1, WritesDone()).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer1, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::UNAVAILABLE, "Something is wrong right now")));

    EXPECT_CALL(*writer2, Write(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer2, WritesDone()).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer2, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::UNAVAILABLE, "Something is wrong right now")));

    EXPECT_THROW(this->upload(content, CASHash::hash(content)), GrpcError);
}

TEST_F(ClientTestFixture, FileTooLargeToBatchUpload)
{
    const auto data = std::string(3 * MAX_BATCH_SIZE_BYTES, '_');
    const Digest digest = CASHash::hash(data);

    Client::UploadRequest request(digest, data);
    std::vector<Client::UploadRequest> requests = {request};

    // Expecting it to fall back to a bytestream Write():
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));
    this->uploadBlobs(requests);
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
        digest.set_hash_other(hashes[i]);
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

    BatchUpdateBlobsRequest request;
    EXPECT_CALL(*casClient.get(), BatchUpdateBlobs(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                        Return(grpc::Status::OK)));
    // And digest in index 2 with the Bytestream API:
    writeResponse.set_committed_size(requests[2].digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    const auto failed_uploads = this->uploadBlobs(requests);
    ASSERT_TRUE(failed_uploads.empty());

    // The client sends the correct instance name:
    EXPECT_EQ(request.instance_name(), client_instance_name);
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
        digest.set_hash_other(hashes[i]);
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

    const auto failed_uploads = this->uploadBlobs(requests);

    ASSERT_EQ(failed_uploads.size(), 2);
    ASSERT_EQ(std::count(hashes.cbegin(), hashes.cend(),
                         failed_uploads[0].digest.hash_other()),
              1);
    ASSERT_EQ(std::count(hashes.cbegin(), hashes.cend(),
                         failed_uploads[1].digest.hash_other()),
              1);
}

TEST_F(ClientTestFixture, CaptureDirectory)
{
    const std::string path_to_capture = "/path/to/stage";
    std::vector<std::string> paths = {path_to_capture};
    const std::string property = "mtime";
    const std::vector<std::string> properties = {property};

    CaptureTreeResponse response;
    auto entry = response.add_responses();
    entry->set_path(path_to_capture);
    entry->mutable_tree_digest()->CopyFrom(make_digest("tree-blob"));
    entry->mutable_status()->set_code(grpc::StatusCode::OK);

    CaptureTreeRequest request;

    EXPECT_CALL(*localCasClient.get(), CaptureTree(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                        Return(grpc::Status::OK)));

    const CaptureTreeResponse returned_response =
        this->captureTree(paths, properties, false);

    // Checking that the request has the data we expect:
    ASSERT_EQ(request.path_size(), 1);
    ASSERT_EQ(request.path(0), path_to_capture);
    ASSERT_EQ(request.node_properties_size(), 1);
    ASSERT_EQ(request.node_properties(0), property);
    ASSERT_FALSE(request.bypass_local_cache());
    ASSERT_EQ(request.instance_name(), this->instanceName());

    // Checking the response:
    ASSERT_EQ(response.responses_size(), 1);
    ASSERT_EQ(response.responses(0).path(), path_to_capture);
    ASSERT_EQ(response.responses(0).status().code(), grpc::StatusCode::OK);
}

TEST_F(ClientTestFixture, CaptureDirectoryErrorThrows)
{
    std::vector<std::string> paths = {std::string("/path/to/stage")};
    std::vector<std::string> properties = {std::string("mtime")};

    CaptureTreeResponse response;
    auto entry = response.add_responses();
    entry->set_path("/dev/null");
    entry->mutable_tree_digest()->CopyFrom(make_digest("tree-blob"));
    entry->mutable_status()->set_code(grpc::StatusCode::OK);

    CaptureTreeRequest request;

    // The retry logic throws after running out of tries:
    EXPECT_CALL(*localCasClient.get(), CaptureTree(_, _, _))
        .WillRepeatedly(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                              Return(grpc::Status(grpc::StatusCode::UNKNOWN,
                                                  "Something went wrong."))));

    ASSERT_THROW(this->captureTree(paths, properties, false),
                 std::runtime_error);
}

TEST_F(ClientTestFixture, CaptureFiles)
{

    const std::vector<std::string> files_to_capture = {
        "/path/to/stage/file1.txt", "/path/to/stage/file2.txt"};

    const std::string property = "mtime";
    const auto mtime = TimeUtils::now();

    // Response that the server will return to the client:
    CaptureFilesResponse response;
    auto entry1 = response.add_responses();
    entry1->set_path(files_to_capture[0]);
    entry1->mutable_digest()->CopyFrom(make_digest("file1.txt-contents"));
    entry1->mutable_status()->set_code(grpc::StatusCode::OK);
    auto properties1 = entry1->mutable_node_properties();
    properties1->mutable_mtime()->CopyFrom(mtime);

    auto entry2 = response.add_responses();
    entry2->set_path(files_to_capture[1]);
    entry2->mutable_digest()->CopyFrom(make_digest("file2.txt-contents"));
    entry2->mutable_status()->set_code(grpc::StatusCode::OK);
    auto properties2 = entry2->mutable_node_properties();
    properties2->mutable_mtime()->CopyFrom(mtime);

    CaptureFilesRequest request;

    EXPECT_CALL(*localCasClient.get(), CaptureFiles(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                        Return(grpc::Status::OK)));

    const std::vector<std::string> properties = {property};

    const CaptureFilesResponse returned_response =
        this->captureFiles(files_to_capture, properties, false);

    // Checking that the request issued contains the data we expect:
    const std::set<std::string> files_to_capture_set(files_to_capture.cbegin(),
                                                     files_to_capture.cend());

    ASSERT_EQ(request.path_size(), 2);
    ASSERT_EQ(files_to_capture_set.count(request.path(0)), 1);
    ASSERT_EQ(files_to_capture_set.count(request.path(1)), 1);
    ASSERT_EQ(request.node_properties_size(), 1);
    ASSERT_EQ(request.node_properties(0), property);

    ASSERT_FALSE(request.bypass_local_cache());

    ASSERT_EQ(request.instance_name(), this->instanceName());

    // Checking that the response returned by the client; it should match the
    // one issued by the server:
    ASSERT_EQ(returned_response.responses_size(), 2);

    ASSERT_NE(returned_response.responses(0).path(),
              returned_response.responses(1).path());

    ASSERT_EQ(
        files_to_capture_set.count(returned_response.responses(0).path()), 1);
    ASSERT_EQ(
        files_to_capture_set.count(returned_response.responses(1).path()), 1);

    ASSERT_EQ(returned_response.responses(0).status().code(),
              grpc::StatusCode::OK);
    ASSERT_EQ(returned_response.responses(1).status().code(),
              grpc::StatusCode::OK);

    ASSERT_EQ(returned_response.responses(0).node_properties().mtime(), mtime);
    ASSERT_EQ(returned_response.responses(1).node_properties().mtime(), mtime);
}

TEST_F(ClientTestFixture, CaptureFilesErrorThrows)
{
    // The retry logic throws after running out of tries:
    EXPECT_CALL(*localCasClient.get(), CaptureFiles(_, _, _))
        .WillOnce(Return(
            grpc::Status(grpc::StatusCode::UNKNOWN, "Something went wrong.")));

    ASSERT_THROW(this->captureFiles({"/path/to/stage/file.txt"}, {}, false),
                 std::runtime_error);
}

TEST_F(ClientTestFixture, FetchTree)
{
    FetchTreeRequest request;
    EXPECT_CALL(*localCasClient.get(), FetchTree(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), Return(grpc::Status::OK)));

    Digest digest;
    digest.set_hash_other("treeHash");
    digest.set_size_bytes(1234);

    ASSERT_NO_THROW(fetchTree(digest, false));

    EXPECT_EQ(request.instance_name(), this->instanceName());
    EXPECT_EQ(request.root_digest(), digest);
    EXPECT_FALSE(request.fetch_file_blobs());
}

TEST_F(ClientTestFixture, FetchTreeWithFiles)
{
    FetchTreeRequest request;
    EXPECT_CALL(*localCasClient.get(), FetchTree(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), Return(grpc::Status::OK)));

    Digest digest;
    digest.set_hash_other("treeHash");
    digest.set_size_bytes(1234);

    ASSERT_NO_THROW(fetchTree(digest, true));

    EXPECT_EQ(request.instance_name(), this->instanceName());
    EXPECT_EQ(request.root_digest(), digest);
    EXPECT_TRUE(request.fetch_file_blobs());
}

TEST_F(ClientTestFixture, FetchTreeFails)
{
    FetchTreeRequest request;
    EXPECT_CALL(*localCasClient.get(), FetchTree(_, _, _))
        .Times(2)
        .WillRepeatedly(DoAll(SaveArg<1>(&request),
                              Return(grpc::Status(grpc::StatusCode::INTERNAL,
                                                  "Something went wrong."))));

    Digest digest;
    digest.set_hash_other("d");
    digest.set_size_bytes(1);

    ASSERT_THROW(fetchTree(digest, false), GrpcError);
    ASSERT_THROW(fetchTree(digest, true), GrpcError);
}

TEST_F(ClientTestFixture, FetchTreeFailsWithRetryableError)
{
    FetchTreeRequest request;
    EXPECT_CALL(*localCasClient.get(), FetchTree(_, _, _))
        .Times(2 * (d_grpcRetryLimit + 1))
        .WillRepeatedly(
            DoAll(SaveArg<1>(&request),
                  Return(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                      "Something went wrong."))));

    Digest digest;
    digest.set_hash_other("d");
    digest.set_size_bytes(1);

    ASSERT_THROW(fetchTree(digest, false), GrpcError);
    ASSERT_THROW(fetchTree(digest, true), GrpcError);
}

class GetTreeFixture : public ClientTestFixture {
  protected:
    Digest d_digest;
    std::vector<buildboxcommon::Directory> d_directories;

    void prepareDigest()
    {
        /* Creates the following directory structure:
         *
         * ./
         *   src/
         *       build.sh*
         *       headers/
         *               file1.h
         *               file2.h
         *               file3.h
         *       cpp/
         *           file1.cpp
         *           file2.cpp
         *           file3.cpp
         *           symlink: file4.cpp --> file3.cpp
         */

        // ./src/headers
        Directory headers_directory;
        std::vector<std::string> headerFiles = {"file1.h", "file2.h",
                                                "file3.h"};
        for (const auto &file : headerFiles) {
            FileNode *fileNode = headers_directory.add_files();
            fileNode->set_name(file);
            fileNode->set_is_executable(false);
            fileNode->mutable_digest()->CopyFrom(
                make_digest(file + "_contents"));
        }
        const auto headers_directory_digest = make_digest(headers_directory);

        // ./src/cpp
        Directory cpp_directory;
        std::vector<std::string> cppFiles = {"file1.cpp", "file2.cpp",
                                             "file3.cpp"};
        for (const auto &file : cppFiles) {
            FileNode *fileNode = cpp_directory.add_files();
            fileNode->set_name(file);
            fileNode->set_is_executable(false);
            fileNode->mutable_digest()->CopyFrom(
                make_digest(file + "_contents"));
        }
        SymlinkNode *symNode = cpp_directory.add_symlinks();
        symNode->set_name("file4.cpp");
        symNode->set_target("file3.cpp");
        const auto cpp_directory_digest = make_digest(cpp_directory);

        // ./src
        Directory src_directory;
        DirectoryNode *headersNode = src_directory.add_directories();
        headersNode->set_name("headers");
        headersNode->mutable_digest()->CopyFrom(headers_directory_digest);
        DirectoryNode *cppNode = src_directory.add_directories();
        cppNode->set_name("cpp");
        cppNode->mutable_digest()->CopyFrom(cpp_directory_digest);
        FileNode *fileNode = src_directory.add_files();
        fileNode->set_name("build.sh");
        fileNode->set_is_executable(true);
        fileNode->mutable_digest()->CopyFrom(make_digest("build.sh_contents"));
        const auto src_directory_digest = make_digest(src_directory);

        // .
        Directory root_directory;
        DirectoryNode *srcNode = root_directory.add_directories();
        srcNode->set_name("src");
        srcNode->mutable_digest()->CopyFrom(src_directory_digest);

        const auto root_directory_serialized =
            root_directory.SerializeAsString();
        d_digest = make_digest(root_directory_serialized);

        d_directories.emplace_back(root_directory);
        d_directories.emplace_back(src_directory);
        d_directories.emplace_back(cpp_directory);
        d_directories.emplace_back(headers_directory);
    }

    GetTreeFixture() { prepareDigest(); }
};

TEST_F(GetTreeFixture, GetTreeSuccess)
{
    // prepare request
    GetTreeRequest request;
    request.set_instance_name("");
    request.mutable_root_digest()->CopyFrom(d_digest);

    EXPECT_CALL(*casClient, GetTreeRaw(_, _))
        .WillOnce(DoAll(SaveArg<1>(&request), Return(gettreereader)));

    // prepare expected response
    GetTreeResponse response;
    for (const auto &d : d_directories) {
        response.add_directories()->CopyFrom(d);
    }

    EXPECT_CALL(*gettreereader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(response), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*gettreereader, Finish()).WillOnce(Return(grpc::Status::OK));

    std::vector<Directory> result = this->getTree(d_digest);
    ASSERT_EQ(result.size(), d_directories.size());
}

TEST_F(GetTreeFixture, GetTreeFail)
{
    EXPECT_CALL(*casClient, GetTreeRaw(_, _)).WillOnce(Return(gettreereader));

    EXPECT_CALL(*gettreereader, Read(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*gettreereader, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::StatusCode::NOT_FOUND,
                         "The root digest was not found in the local CAS.")));

    EXPECT_THROW(this->getTree(d_digest), std::runtime_error);
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
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    WriteRequest request;
    EXPECT_CALL(*writer, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(tmpfile.fd(), digest);
    EXPECT_EQ(request.resource_name().substr(0, client_instance_name.size()),
              client_instance_name);
}

TEST_F(UploadFileFixture, UploadFileCommittedSizeMismatch)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes() - 1);
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    WriteRequest request;
    EXPECT_CALL(*writer, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    ASSERT_THROW(this->upload(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(UploadFileFixture, UploadLargeFileTest)
{
    int contentLength = 3 * 1024 * 1024;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadExactFileTest)
{
    int contentLength = 1024 * 1024;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadJustLargerThanExactFileTest)
{
    int contentLength = 1024 * 1024 + 1;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadJustSmallerThanExactFileTest)
{
    int contentLength = 1024 * 1024 - 1;
    std::string content = std::string(contentLength, 'f');

    // Overwrite the whole file
    lseek(tmpfile.fd(), 0, SEEK_SET);
    write(tmpfile.fd(), content.c_str(), contentLength);

    digest.set_size_bytes(contentLength);
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish()).WillOnce(Return(grpc::Status::OK));

    this->upload(tmpfile.fd(), digest);
}

TEST_F(UploadFileFixture, UploadFileReadFailure)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_THROW(this->upload(-40, digest), std::system_error);
}

TEST_F(UploadFileFixture, UploadFileWriteFailure)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(false));

    EXPECT_THROW(this->upload(tmpfile.fd(), digest), std::runtime_error);
}

TEST_F(UploadFileFixture, UploadAlreadyExistingFile)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    writeResponse.set_committed_size(digest.size_bytes());
    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(writeResponse), Return(writer)));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(grpc::Status(grpc::Status::OK)));

    ASSERT_NO_THROW(this->upload(tmpfile.fd(), digest));
}

TEST_F(UploadFileFixture, UploadFileDidntReturnOk)
{
    digest.set_size_bytes(content.length());
    digest.set_hash_other("fakehash");

    EXPECT_CALL(*bytestreamClient, WriteRaw(_, _)).WillOnce(Return(writer));

    EXPECT_CALL(*writer, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*writer, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*writer, Finish())
        .WillOnce(Return(
            grpc::Status(grpc::FAILED_PRECONDITION, "failing for test")));

    EXPECT_THROW(this->upload(tmpfile.fd(), digest), std::runtime_error);
}

class TransferDirectoryFixture : public ClientTestFixture {
    /**
     * Instantiates a tempfile with some data for use in upload tests.
     * Inherits from the pre-instantiated client fixture.
     */
  protected:
    /* directory/
     *   |-- file_a
     *   |-- subdirectory/
     *       |-- file_b
     */

    TransferDirectoryFixture()
        : ClientTestFixture(max_batch_size_bytes), directory(),
          subdirectory(directory.name(), "tmp-subdir"),
          file_a(directory.name(), "test-tmp-file"),
          file_b(subdirectory.name(), "test-tmp-file")
    {
        std::ofstream file_a_stream(file_a.name());
        file_a_stream << file_a_contents;
        file_a_stream.close();

        std::ofstream file_b_stream(file_b.name());
        file_b_stream << file_b_contents;
        file_b_stream.close();

        nested_directory =
            make_nesteddirectory(directory.name(), &directory_file_map);
        directory_digest = nested_directory.to_digest(&directory_file_map);
        serialized_directory = directory_file_map.at(directory_digest);
    }

    // Setting a larger value so that we are allowed to upload complete
    // directories in a single batch
    static const int64_t max_batch_size_bytes = 1024 * 1024;

    TemporaryDirectory directory, subdirectory;
    TemporaryFile file_a, file_b;

    const std::string file_a_contents = "Hello world!";
    const std::string file_b_contents = "This is some data...";

    const Digest file_a_digest = make_digest(file_a_contents);
    const Digest file_b_digest = make_digest(file_b_contents);

    digest_string_map directory_file_map;
    NestedDirectory nested_directory;
    std::string serialized_directory;
    Digest directory_digest;
};

TEST_F(TransferDirectoryFixture, UploadDirectory)
{
    // We expect the client to check if there are any blobs are missing to
    // avoid transferring those. For this test, we'll mock that all are missing
    // in the remote.

    // 1) FindMissingBlobs()
    FindMissingBlobsResponse missing_blobs_response;
    for (const auto &entry : directory_file_map) {
        auto missing_digest =
            missing_blobs_response.add_missing_blob_digests();
        missing_digest->CopyFrom(entry.first);
    }

    EXPECT_CALL(*casClient.get(), FindMissingBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(missing_blobs_response),
                        Return(grpc::Status::OK)));

    BatchUpdateBlobsRequest update_request;
    BatchUpdateBlobsResponse update_response;

    // 2) BatchUpdateBlobs()
    EXPECT_CALL(*casClient.get(), BatchUpdateBlobs(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&update_request),
                        SetArgPointee<2>(update_response),
                        Return(grpc::Status::OK)));

    // We return success for all the updates:
    for (const auto &entry : directory_file_map) {
        const Digest file_digest = entry.first;
        auto response = update_response.add_responses();
        response->mutable_digest()->CopyFrom(file_digest);
        response->mutable_status()->set_code(grpc::StatusCode::OK);
    }

    Digest returned_directory_digest;
    this->uploadDirectory(std::string(directory.name()),
                          &returned_directory_digest);

    ASSERT_EQ(returned_directory_digest, directory_digest);

    // All the data was written to the remote:
    ASSERT_EQ(update_request.requests_size(), directory_file_map.size());
    for (const auto &entry : update_request.requests()) {
        ASSERT_EQ(directory_file_map.count(entry.digest()), 1);
    }
}

TEST_F(TransferDirectoryFixture, UploadDirectoryNoMissingBlobs)
{
    // In this test the remote reports that no blobs are missing, so no upload
    // needs to take place.

    FindMissingBlobsResponse missing_blobs_response;

    EXPECT_CALL(*casClient.get(), FindMissingBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(missing_blobs_response),
                        Return(grpc::Status::OK)));

    Digest returned_directory_digest;
    this->uploadDirectory(std::string(directory.name()),
                          &returned_directory_digest);

    ASSERT_EQ(returned_directory_digest, directory_digest);
}

TEST_F(TransferDirectoryFixture, UploadDirectoryWritesTree)
{
    /* directory/
     *   |-- file_a
     *   |-- subdirectory/
     *       |-- file_b
     */

    FindMissingBlobsResponse missing_blobs_response;
    // The remote reports that no blobs are missing, so no upload
    // needs to take place.
    EXPECT_CALL(*casClient.get(), FindMissingBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(missing_blobs_response),
                        Return(grpc::Status::OK)));

    Digest returned_directory_digest;
    Tree tree;
    this->uploadDirectory(std::string(directory.name()),
                          &returned_directory_digest, &tree);

    // `uploadDirectory()` wrote the `Tree` object that it generated before the
    // transfer, it should match the data:

    // directory/:
    ASSERT_EQ(tree.root().directories_size(), 1); // subdirectory/
    ASSERT_EQ(tree.children_size(), 1);

    ASSERT_EQ(tree.root().files_size(), 1); // fila_a
    ASSERT_EQ(tree.root().symlinks_size(), 0);

    // subdirectory/
    ASSERT_EQ(tree.children(0).files_size(), 1); // file_b
    ASSERT_EQ(tree.children(0).directories_size(), 0);
    ASSERT_EQ(tree.children(0).symlinks_size(), 0);

    ASSERT_EQ(returned_directory_digest, directory_digest);
}

TEST_F(ClientTestFixture, DownloadDirectoryTestActualDownload)
{
    TemporaryDirectory capture_dir;
    TemporaryDirectory write_dir;
    digest_string_map directory_file_map;

    auto file_in_capture_dir = std::string(capture_dir.name()) + "/file1";
    auto symlink_in_capture_dir =
        std::string(capture_dir.name()) + "/" + "symlink1";
    auto symlink_in_write_dir =
        std::string(write_dir.name()) + "/" + "symlink1";
    auto file_in_write_dir = std::string(write_dir.name()) + "/file1";

    // create the file in the tempdir: capture_dir
    buildboxcommontest::TestUtils::touchFile(file_in_capture_dir.c_str());
    // make a symlink to the file
    EXPECT_TRUE(symlink(file_in_capture_dir.c_str(),
                        symlink_in_capture_dir.c_str()) == 0);

    NestedDirectory nested_directory =
        make_nesteddirectory(capture_dir.name(), &directory_file_map);
    Digest directory_digest = nested_directory.to_digest(&directory_file_map);

    // this callback, if called, will create the file in the write directory.
    download_callback_t download_blobs =
        [&](const std::vector<Digest> &file_digests,
            const OutputMap &outputs) {
            buildboxcommontest::TestUtils::touchFile(
                file_in_write_dir.c_str());
        };

    // return the Directory Node from the map created in make_nesteddirectory
    return_directory_callback_t download_directory =
        [&](const Digest &digest) {
            Directory dir;
            const auto dir_string = directory_file_map.at(directory_digest);
            dir.ParseFromString(dir_string);
            return dir;
        };

    this->downloadDirectory(directory_digest, write_dir.name(), download_blobs,
                            download_directory);

    // verify that write_dir has the same contents as capture_dir
    ASSERT_FALSE(FileUtils::directoryIsEmpty(write_dir.name()));
    ASSERT_TRUE(FileUtils::isDirectory(write_dir.name()));
    ASSERT_TRUE(FileUtils::isRegularFile(file_in_write_dir.c_str()));
    ASSERT_TRUE(FileUtils::isSymlink(symlink_in_write_dir.c_str()));
}

TEST_F(TransferDirectoryFixture, DownloadDirectoryMissingDigestThrows)
{
    Digest digest;
    digest.set_hash_other("ThisDoesNotExist");
    digest.set_size_bytes(1234);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));

    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    EXPECT_CALL(*reader, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::NOT_FOUND,
                                      "Blob not found in CAS")));

    TemporaryDirectory output_dir;
    ASSERT_THROW(this->downloadDirectory(digest, output_dir.name()),
                 std::runtime_error);
}

TEST_F(TransferDirectoryFixture, StageDirectory)
{
    EXPECT_CALL(*localCasClient.get(), StageTreeRaw(_))
        .WillOnce(Return(reader_writer));

    // The client will issue 2 requests: the actual `StageTreeRequest` and an
    // empty message to indicate to the server that it can clean up.
    EXPECT_CALL(*reader_writer, Write(_, _))
        .Times(2)
        .WillRepeatedly(Return(true));

    StageTreeResponse response;
    response.set_path("/tmp/stage/");
    EXPECT_CALL(*reader_writer, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(response), Return(true)));

    {
        std::unique_ptr<StagedDirectory> staged_dir =
            this->stage(directory_digest, "/tmp/stage");
        ASSERT_EQ(staged_dir->path(), "/tmp/stage/");

        // The StagedDirectory destructor automatically sends the second empty
        // message to the server.
        EXPECT_CALL(*reader_writer, WritesDone()).WillOnce(Return(true));
    }
}

TEST_F(TransferDirectoryFixture, StageDirectoryThrowsOnError)
{
    EXPECT_CALL(*localCasClient.get(), StageTreeRaw(_))
        .WillOnce(Return(reader_writer));

    EXPECT_CALL(*reader_writer, Write(_, _)).WillOnce(Return(false));

    EXPECT_CALL(*reader_writer, Finish()).WillOnce(Return(grpc::Status::OK));

    EXPECT_THROW(this->stage(directory_digest, "/tmp/stage"),
                 std::runtime_error);
}

class DownloadBlobsFixture : public ClientTestFixture,
                             public ::testing::WithParamInterface<bool> {
  protected:
    DownloadBlobsFixture() {}

    /* This fixture tests
     * `void Client::downloadBlobs(const std::vector<Digest> &digests,
     *                             const write_blob_callback_t &write_blob,
     *                             bool throw_on_error).
     *
     * That `protected` helper is shared by the other public-facing
     * `downloadBlobs()` versions, so this allows to reuse tests.
     *
     * The parameterized `bool` value of this fixture will be passed to the
     * `throw_on_error` parameter.
     */
};

INSTANTIATE_TEST_CASE_P(DownloadBlobsTestCase, DownloadBlobsFixture,
                        testing::Bool());
TEST_P(DownloadBlobsFixture, FileTooLargeToBatchDownload)
{
    // Expecting it to fall back to a bytestream Read():
    const auto data = std::string(2 * MAX_BATCH_SIZE_BYTES, '-');
    const Digest digest = CASHash::hash(data);

    const std::vector<Digest> requests = {digest};

    readResponse.set_data(data);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));

    auto write_blob = [&](const std::string &downloaded_hash,
                          const std::string &downloaded_data) {
        ASSERT_EQ(downloaded_hash, digest.hash_other());
        ASSERT_EQ(downloaded_data, data);
    };

    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    const bool throw_on_error = GetParam();
    ASSERT_NO_THROW(
        this->downloadBlobs(requests, write_blob, nullptr, throw_on_error));
}

TEST_P(DownloadBlobsFixture, DownloadBlobs)
{
    const std::vector<std::string> payload = {
        "a", "b", std::string(3 * MAX_BATCH_SIZE_BYTES, 'x'), "c"};
    std::vector<std::string> hashes;

    // Creating list of requests...
    std::vector<Digest> requests;
    for (unsigned i = 0; i < payload.size(); i++) {
        const Digest digest = CASHash::hash(payload[i]);
        hashes.push_back(digest.hash_other());
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

    BatchReadBlobsRequest request;
    EXPECT_CALL(*casClient.get(), BatchReadBlobs(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                        Return(grpc::Status::OK)));

    // And digest in index 2 with the Bytestream API:
    readResponse.set_data(payload[2]);
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    // We will write the output results into a map indexed by hash:
    std::unordered_map<std::string, std::string> downloads;
    auto write_blob = [&](const std::string &hash, const std::string &data) {
        downloads[hash] = data;
    };

    const bool throw_on_error = GetParam();
    Client::DownloadResults download_results;
    ASSERT_NO_THROW(download_results = this->downloadBlobs(
                        requests, write_blob, nullptr, throw_on_error));

    // Client sends the correct instance name:
    EXPECT_EQ(request.instance_name(), client_instance_name);

    // We get all the data, and it's correct for each requested digest:
    ASSERT_EQ(downloads.size(), requests.size());
    ASSERT_EQ(downloads.at(hashes[0]), payload[0]);
    ASSERT_EQ(downloads.at(hashes[1]), payload[1]);
    ASSERT_EQ(downloads.at(hashes[2]), payload[2]);
    ASSERT_EQ(downloads.at(hashes[3]), payload[3]);

    // The returned dictionary has whether or not each digest was sucessfully
    // fetched:
    ASSERT_EQ(download_results.size(), 4);
    for (const auto &digest : requests) {
        const auto result = std::find_if(
            download_results.cbegin(), download_results.cend(),
            [&digest](const DownloadResult &r) { return r.first == digest; });

        ASSERT_NE(result, download_results.cend());
        ASSERT_EQ(result->second.code(), grpc::StatusCode::OK);
    }
}

TEST_P(DownloadBlobsFixture, DownloadBlobsBatchWithMissingBlob)
{
    const Digest existing_digest = CASHash::hash("existing-blob");
    const Digest non_existing_digest = CASHash::hash("missing-blob");

    BatchReadBlobsResponse response;
    auto entry1 = response.add_responses();
    entry1->mutable_digest()->CopyFrom(non_existing_digest);
    entry1->mutable_status()->set_code(grpc::StatusCode::NOT_FOUND);
    entry1->mutable_status()->set_message("Digest not found in CAS.");

    auto entry2 = response.add_responses();
    entry2->mutable_digest()->CopyFrom(existing_digest);
    entry2->mutable_status()->set_code(grpc::StatusCode::OK);
    entry2->set_data("existing-blob");

    EXPECT_CALL(*casClient.get(), BatchReadBlobs(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(response), Return(grpc::Status::OK)));

    unsigned int written_blobs = 0;
    const auto write_blob = [&](const std::string &hash, const std::string &) {
        ASSERT_EQ(hash, existing_digest.hash_other());
        written_blobs++;
    };

    const bool throw_on_error = GetParam();

    std::vector<Digest> requests = {existing_digest, non_existing_digest};
    Client::DownloadResults download_results;
    ASSERT_NO_THROW(download_results = this->downloadBlobs(
                        requests, write_blob, nullptr, throw_on_error));
    ASSERT_EQ(written_blobs, 1);

    ASSERT_EQ(download_results.size(), 2);

    for (const auto &entry : download_results) {
        const Digest digest = entry.first;
        const google::rpc::Status status = entry.second;

        if (digest == existing_digest) {
            ASSERT_EQ(status.code(), grpc::StatusCode::OK);
        }
        else if (digest == non_existing_digest) {
            ASSERT_EQ(status.code(), grpc::StatusCode::NOT_FOUND);
        }
        else {
            FAIL() << "Unexpected digest in response: [" << digest
                   << "] was not requested.";
        }
    }
}

TEST_P(DownloadBlobsFixture, DownloadBlobsHelperFails)
{
    Digest digest;
    digest.set_hash_other("hash0");
    digest.set_size_bytes(3 * MAX_BATCH_SIZE_BYTES);

    unsigned int written_blobs = 0;
    const auto write_blob = [&](const std::string &, const std::string &) {
        written_blobs++;
    };

    const auto errorStatus =
        grpc::Status(grpc::StatusCode::NOT_FOUND, "Digest not found in CAS.");
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(errorStatus));

    const bool throw_on_error = GetParam();
    if (throw_on_error) {
        ASSERT_THROW(
            this->downloadBlobs({digest}, write_blob, nullptr, throw_on_error),
            std::runtime_error);
        // With the current implementation there are no guarantees about
        // the data written before an error is encountered and the method
        // aborts.
    }
    else {
        Client::DownloadResults download_results;
        ASSERT_NO_THROW(download_results = this->downloadBlobs(
                            {digest}, write_blob, nullptr, throw_on_error));

        ASSERT_EQ(written_blobs, 0);

        // The returned vector contains that the requested digest failed
        // to be downloaded:
        ASSERT_EQ(download_results.size(), 1);
        ASSERT_EQ(download_results.front().first, digest);
        ASSERT_EQ(download_results.front().second.code(),
                  errorStatus.error_code());
    }
}

TEST_F(DownloadBlobsFixture, DownloadBlobsResultSuccessfulStatusAndData)
{
    // Test the public `downloadBlobs()` method to check that the returned map
    // is correct.
    const auto data = std::string(MAX_BATCH_SIZE_BYTES + 1, 'A');
    readResponse.set_data(data);

    const Digest digest = CASHash::hash(data);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    const Client::DownloadBlobsResult download_results =
        this->downloadBlobs({digest});
    ASSERT_EQ(download_results.count(digest.hash_other()), 1);
    ASSERT_EQ(download_results.size(), 1);

    const auto &result = download_results.at(digest.hash_other());
    const auto &result_status = result.first;
    const auto &result_data = result.second;

    ASSERT_EQ(result_status.code(), grpc::StatusCode::OK);
    ASSERT_EQ(result_data, data);
}

TEST_F(DownloadBlobsFixture, DownloadBlobsResultErrorCode)
{
    // Test the public `downloadBlobs()` method to check that the returned map
    // is correct.
    Digest digest;
    digest.set_hash_other("hash0");
    digest.set_size_bytes(3 * MAX_BATCH_SIZE_BYTES);

    const auto errorStatus =
        grpc::Status(grpc::StatusCode::NOT_FOUND, "Digest not found in CAS.");
    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(errorStatus));

    Client::DownloadBlobsResult download_results;
    ASSERT_NO_THROW(download_results = this->downloadBlobs({digest}));

    ASSERT_EQ(download_results.size(), 1);
    ASSERT_EQ(download_results.count(digest.hash_other()), 1);

    const auto result = download_results.at(digest.hash_other());
    ASSERT_EQ(result.first.code(), errorStatus.error_code());
    ASSERT_EQ(result.second, "");
}

TEST_F(DownloadBlobsFixture,
       DownloadBlobsToDirectoryResultSuccessfulStatusAndPath)
{
    // Test the public `downloadBlobsToDirectory()` method to check that the
    // returned map is correct.
    const auto data = std::string(MAX_BATCH_SIZE_BYTES + 1, 'A');
    readResponse.set_data(data);

    const Digest digest = CASHash::hash(data);

    EXPECT_CALL(*bytestreamClient, ReadRaw(_, _)).WillOnce(Return(reader));
    EXPECT_CALL(*reader, Read(_))
        .WillOnce(DoAll(SetArgPointee<0>(readResponse), Return(true)))
        .WillOnce(Return(false));
    EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));

    TemporaryDirectory directory;

    const Client::DownloadBlobsResult download_results =
        this->downloadBlobsToDirectory({digest}, directory.name());
    ASSERT_EQ(download_results.count(digest.hash_other()), 1);
    ASSERT_EQ(download_results.size(), 1);

    const auto &result = download_results.at(digest.hash_other());
    const auto &result_status = result.first;
    const auto &result_path = result.second;

    const auto result_data = FileUtils::getFileContents(result_path.c_str());

    ASSERT_EQ(result_status.code(), grpc::StatusCode::OK);
    ASSERT_EQ(result_data, data);
}
