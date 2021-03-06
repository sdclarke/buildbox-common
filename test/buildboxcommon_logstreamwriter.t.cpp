/*
 * Copyright 2020 Bloomberg Finance LP
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

#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_logstreamwriter.h>
#include <buildboxcommon_protos.h>

#include <memory>

#include <build/bazel/remote/logstream/v1/remote_logstream_mock.grpc.pb.h>
#include <google/bytestream/bytestream_mock.grpc.pb.h>
#include <grpcpp/test/mock_stream.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;
using namespace testing;

class LogStreamWriterTestFixture : public testing::Test {
  protected:
    typedef grpc::testing::MockClientWriter<google::bytestream::WriteRequest>
        MockClientWriter;

    const std::string TESTING_RESOURCE_NAME = "dummy-resource-name";
    const int GRPC_RETRY_LIMIT = 3;
    const int GRPC_RETRY_DELAY = 1;

    LogStreamWriterTestFixture()
        : byteStreamClient(
              std::make_shared<google::bytestream::MockByteStreamStub>()),
          logStreamWriter(TESTING_RESOURCE_NAME, byteStreamClient,
                          GRPC_RETRY_LIMIT, GRPC_RETRY_DELAY),
          mockClientWriter(new MockClientWriter()),
          logStreamClient(std::make_unique<build::bazel::remote::logstream::
                                               v1::MockLogStreamServiceStub>())

    {
    }

    ~LogStreamWriterTestFixture() {}

    std::shared_ptr<google::bytestream::MockByteStreamStub> byteStreamClient;
    buildboxcommon::LogStreamWriter logStreamWriter;

    MockClientWriter *mockClientWriter;
    // Wrapping this in a smart pointer causes issues on destruction.
    // (It is freed by someone else.)

    std::unique_ptr<
        build::bazel::remote::logstream::v1::MockLogStreamServiceStub>
        logStreamClient;
};

TEST_F(LogStreamWriterTestFixture, TestSuccessfulWrite)
{
    const std::string data = "Hello!!";

    // Initial `QueryWriteStatus()` request on first call to `write()`:
    EXPECT_CALL(*byteStreamClient, QueryWriteStatus(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    WriteResponse response;
    response.set_committed_size(
        static_cast<google::protobuf::int64>(data.size()));

    WriteRequest request;
    EXPECT_CALL(*byteStreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(response), Return(mockClientWriter)));

    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));

    EXPECT_TRUE(logStreamWriter.write(data));

    EXPECT_FALSE(request.finish_write());
    EXPECT_EQ(request.resource_name(), TESTING_RESOURCE_NAME);
}

TEST_F(LogStreamWriterTestFixture, TestWriteFailsWithUncommitedData)
{
    // Initial `QueryWriteStatus()` request on first call to `write()`:
    EXPECT_CALL(*byteStreamClient, QueryWriteStatus(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    WriteResponse response;
    response.set_committed_size(0);

    WriteRequest request;
    EXPECT_CALL(*byteStreamClient, WriteRaw(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(response), Return(mockClientWriter)));

    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));

    EXPECT_TRUE(logStreamWriter.write("ABCD"));

    EXPECT_CALL(*mockClientWriter, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, Finish())
        .WillOnce(Return(grpc::Status::OK));
    EXPECT_FALSE(logStreamWriter.commit());
}

TEST_F(LogStreamWriterTestFixture, TestMultipleWritesAndCommit)
{
    const std::string data1 = "This is the first part...";
    const std::string data2 = "Second part.";

    // Initial `QueryWriteStatus()` request on first call to `write()`:
    EXPECT_CALL(*byteStreamClient, QueryWriteStatus(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    WriteResponse write_response;
    write_response.set_committed_size(
        static_cast<google::protobuf::int64>(data1.size()) +
        static_cast<google::protobuf::int64>(data2.size()));

    WriteRequest request1;
    EXPECT_CALL(*byteStreamClient, WriteRaw(_, _))
        .WillOnce(
            DoAll(SetArgPointee<1>(write_response), Return(mockClientWriter)));

    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request1), Return(true)));

    EXPECT_NO_THROW(logStreamWriter.write(data1));
    EXPECT_FALSE(request1.finish_write());
    EXPECT_EQ(request1.data(), data1);
    EXPECT_EQ(request1.write_offset(), 0);

    WriteRequest request2;
    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request2), Return(true)));

    EXPECT_NO_THROW(logStreamWriter.write(data2));
    EXPECT_FALSE(request2.finish_write());
    EXPECT_EQ(request2.data(), data2);
    EXPECT_EQ(request2.write_offset(), data1.size());

    // Calling `commit()`:
    WriteRequest commit_request;
    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&commit_request), Return(true)));
    EXPECT_CALL(*mockClientWriter, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_TRUE(logStreamWriter.commit());
    EXPECT_TRUE(commit_request.finish_write());
    EXPECT_EQ(commit_request.write_offset(), data1.size() + data2.size());
}

TEST_F(LogStreamWriterTestFixture, TestFinishWrite)
{
    EXPECT_CALL(*byteStreamClient, WriteRaw(_, _))
        .WillOnce(Return(mockClientWriter));

    WriteRequest request;
    EXPECT_CALL(*mockClientWriter, Write(_, _))
        .WillOnce(DoAll(SaveArg<0>(&request), Return(true)));

    EXPECT_CALL(*mockClientWriter, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_TRUE(logStreamWriter.commit());
    EXPECT_TRUE(request.finish_write());
    EXPECT_EQ(request.resource_name(), TESTING_RESOURCE_NAME);
}

TEST_F(LogStreamWriterTestFixture, TestOperationsAfterCommitThrow)
{
    EXPECT_CALL(*byteStreamClient, WriteRaw(_, _))
        .WillOnce(Return(mockClientWriter));
    EXPECT_CALL(*mockClientWriter, Write(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, WritesDone()).WillOnce(Return(true));
    EXPECT_CALL(*mockClientWriter, Finish())
        .WillOnce(Return(grpc::Status::OK));

    EXPECT_TRUE(logStreamWriter.commit());
    EXPECT_THROW(logStreamWriter.write("More data"), std::runtime_error);
    EXPECT_THROW(logStreamWriter.commit(), std::runtime_error);
}

TEST_F(LogStreamWriterTestFixture, TestQueryWriteStatusReturnsNotFound)
{
    const std::string data = "Hello!!";
    // The `QueryWriteStatus()` request before performing a
    // `ByteStream.Write()` returns `NOT_FOUND`. This means we cannot write to
    // the stream.
    EXPECT_CALL(*byteStreamClient, QueryWriteStatus(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::NOT_FOUND, "")));

    EXPECT_FALSE(logStreamWriter.write(data));
}

TEST_F(LogStreamWriterTestFixture, SuccessfulCreateLogStream)
{
    const std::string parent = "parent";
    const std::string readName = parent + "/foo";
    const std::string writeName = parent + "/foo/WRITE";

    LogStream response;
    response.set_name(readName);
    response.set_write_resource_name(writeName);

    CreateLogStreamRequest request;
    EXPECT_CALL(*logStreamClient, CreateLogStream(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&request), SetArgPointee<2>(response),
                        Return(grpc::Status::OK)));

    const LogStream returnedLogStream = LogStreamWriter::createLogStream(
        parent, GRPC_RETRY_LIMIT, GRPC_RETRY_DELAY, logStreamClient.get());

    // The request contains the parent value we specified:
    EXPECT_EQ(request.parent(), parent);

    // And the returned LogStream matches the one sent by the server:
    EXPECT_EQ(returnedLogStream.name(), response.name());
    EXPECT_EQ(returnedLogStream.write_resource_name(),
              response.write_resource_name());
}

TEST_F(LogStreamWriterTestFixture, CreateLogStreamReturnsError)
{
    const grpc::Status errorStatus = grpc::Status(
        grpc::StatusCode::UNAVAILABLE, "LogStream server is taking a nap.");

    EXPECT_CALL(*logStreamClient, CreateLogStream(_, _, _))
        .WillRepeatedly(Return(errorStatus));

    ASSERT_THROW(LogStreamWriter::createLogStream("parent", GRPC_RETRY_LIMIT,
                                                  GRPC_RETRY_DELAY,
                                                  logStreamClient.get()),
                 GrpcError);
}
