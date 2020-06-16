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
#include <buildboxcommon_logstreamwriter.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_logging.h>

#include <sstream>

namespace buildboxcommon {

LogStreamWriter::LogStreamWriter(const std::string &resourceName,
                                 const ConnectionOptions &connectionOptions)
    : LogStreamWriter(resourceName,
                      ByteStream::NewStub(connectionOptions.createChannel()),
                      std::stoi(connectionOptions.d_retryLimit),
                      std::stoi(connectionOptions.d_retryDelay))
{
}

LogStreamWriter::LogStreamWriter(
    const std::string &resourceName,
    std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
    const int grpcRetryLimit, const int grpcRetryDelay)
    : d_resourceName(resourceName), d_grpcRetryLimit(grpcRetryLimit),
      d_grpcRetryDelay(grpcRetryDelay), d_byteStreamClient(bytestreamClient),
      d_writeOffset(0), d_writeCommitted(false)
{
}

void LogStreamWriter::write(const std::string &data)
{
    if (d_writeCommitted) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Attempted to `write()` after `commit()`.")
    }

    WriteRequest request;
    request.set_resource_name(d_resourceName);
    request.set_write_offset(d_writeOffset);
    request.set_data(data);
    request.set_finish_write(false);

    auto writeLambda = [&request, this](grpc::ClientContext &) {
        if (!bytestreamWriter()->Write(request)) {
            const auto errorMessage = "Upload failed: broken stream";
            BUILDBOX_LOG_DEBUG(errorMessage);
            return grpc::Status(grpc::StatusCode::INTERNAL, errorMessage);
        }

        return grpc::Status::OK;
    };

    GrpcRetry::retry(writeLambda, d_grpcRetryLimit, d_grpcRetryDelay);

    d_writeOffset += data.size();
}

void LogStreamWriter::commit()
{
    if (d_writeCommitted) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Attempted to `commit()` an already commited write.")
    }

    WriteRequest request;
    request.set_resource_name(d_resourceName);
    request.set_write_offset(d_writeOffset);
    request.set_finish_write(true);

    auto commitWriteLambda = [&request, this](grpc::ClientContext &) {
        if (!bytestreamWriter()->Write(request)) {
            const auto errorMessage = "Upload failed: broken stream";
            BUILDBOX_LOG_DEBUG(errorMessage);
            return grpc::Status(grpc::StatusCode::INTERNAL, errorMessage);
        }

        bytestreamWriter()->WritesDone();

        const auto status = bytestreamWriter()->Finish();
        if (status.ok()) {
            const auto bytesWritten = d_writeOffset;
            if (d_writeResponse.committed_size() != bytesWritten) {
                std::stringstream errorMessage;
                errorMessage << "Server reported uncommitted data: "
                             << d_writeResponse.committed_size() << " of "
                             << bytesWritten << " bytes";
                BUILDBOX_LOG_DEBUG(errorMessage.str());
                return grpc::Status(grpc::StatusCode::DATA_LOSS,
                                    errorMessage.str());
            }
        }

        return status;
    };

    GrpcRetry::retry(commitWriteLambda, d_grpcRetryLimit, d_grpcRetryDelay);

    d_writeCommitted = true;
}

LogStreamWriter::ByteStreamClientWriter &LogStreamWriter::bytestreamWriter()
{
    if (d_bytestreamWriter == nullptr) {
        d_bytestreamWriter =
            d_byteStreamClient->Write(&d_clientContext, &d_writeResponse);
    }

    return d_bytestreamWriter;
}

} // namespace buildboxcommon
