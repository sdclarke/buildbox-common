#include <buildboxcommon_grpcretry.h>

#include <gtest/gtest.h>

#include <iostream>

using buildboxcommon::grpcRetry;

TEST(GrpcRetry, SimpleSucceedTest)
{
    int failures = 0;
    int retryLimit = 1;
    int retryDelay = 100;

    /* Suceed once, if called again fail */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 1) {
            failures++;
            return grpc::Status::OK;
        }
        else {
            return grpc::Status(grpc::FAILED_PRECONDITION, "failing in test");
        }
    };

    EXPECT_NO_THROW(grpcRetry(lambda, retryLimit, retryDelay));
}

TEST(GrpcRetry, SimpleRetrySucceedTest)
{
    int failures = 0;
    int retryLimit = 1;
    int retryDelay = 100;

    /* Fail once, then succeed. */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 1) {
            failures++;
            return grpc::Status(grpc::FAILED_PRECONDITION, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_NO_THROW(grpcRetry(lambda, retryLimit, retryDelay));
}

TEST(GrpcRetry, SimpleRetryFailTest)
{
    int failures = 0;
    int retryLimit = 2;
    int retryDelay = 100;

    /* Fail three times, then succeed. */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 3) {
            failures++;
            return grpc::Status(grpc::FAILED_PRECONDITION, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_THROW(grpcRetry(lambda, retryLimit, retryDelay),
                 std::runtime_error);
}
