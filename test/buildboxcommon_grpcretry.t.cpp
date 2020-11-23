#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_requestmetadata.h>

#include <gtest/gtest.h>

#include <functional>
#include <iostream>

using buildboxcommon::GrpcRetry;

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
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
    };

    EXPECT_NO_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay));

    failures = 0;
    EXPECT_NO_THROW(
        GrpcRetry::retry(lambda, "lambda()", retryLimit, retryDelay));
}

TEST(GrpcRetry, OtherException)
{
    int failures = 0;
    int retryLimit = 1;
    int retryDelay = 100;
    buildboxcommon::GrpcStatusCodes otherExceptions = {
        grpc::DEADLINE_EXCEEDED};

    /* Suceed once, if called again fail */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 1) {
            failures++;
            return grpc::Status(grpc::DEADLINE_EXCEEDED, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    const auto f = [](grpc::ClientContext *) { return; };
    EXPECT_NO_THROW(GrpcRetry::retry(lambda, "", retryLimit, retryDelay, f,
                                     otherExceptions));

    failures = -1;
    EXPECT_THROW(GrpcRetry::retry(lambda, "lambda()", retryLimit, retryDelay,
                                  f, otherExceptions),
                 std::runtime_error);
}

TEST(GrpcRetry, MultipleException)
{
    int failures = 0;
    int retryLimit = 3;
    int retryDelay = 100;
    buildboxcommon::GrpcStatusCodes otherExceptions = {grpc::DEADLINE_EXCEEDED,
                                                       grpc::INVALID_ARGUMENT};

    /* Succeed once, if called again fail */
    auto lambda = [&](grpc::ClientContext &context) {
        switch (failures) {
            case 0:
                failures++;
                return grpc::Status(grpc::DEADLINE_EXCEEDED,
                                    "failing in test");
            case 1:
                failures++;
                return grpc::Status(grpc::INVALID_ARGUMENT, "failing in test");
            case 2:
                failures++;
                return grpc::Status(grpc::UNAVAILABLE, "failing in test");
            case 3:
                return grpc::Status::OK;
        }
        return grpc::Status::OK;
    };

    const auto f = [](grpc::ClientContext *) { return; };
    EXPECT_NO_THROW(GrpcRetry::retry(lambda, "", retryLimit, retryDelay, f,
                                     otherExceptions));

    failures = 0;
    retryLimit = 2;
    EXPECT_THROW(GrpcRetry::retry(lambda, "lambda()", retryLimit, retryDelay,
                                  f, otherExceptions),
                 std::runtime_error);
}

TEST(GrpcRetry, ExceptionNotIncluded)
{
    int failures = 0;
    int retryLimit = 3;
    int retryDelay = 100;
    buildboxcommon::GrpcStatusCodes otherExceptions = {grpc::DEADLINE_EXCEEDED,
                                                       grpc::INVALID_ARGUMENT};

    /* Suceed once, if called again fail */
    auto lambda = [&](grpc::ClientContext &context) {
        switch (failures) {
            case 0:
                failures++;
                return grpc::Status(grpc::DEADLINE_EXCEEDED,
                                    "failing in test");
            case 1:
                failures++;
                return grpc::Status(grpc::INVALID_ARGUMENT, "failing in test");
            case 2:
                failures++;
                return grpc::Status(grpc::PERMISSION_DENIED,
                                    "failing in test");
            case 3:
                return grpc::Status::OK;
        }
        return grpc::Status::OK;
    };

    const auto f = [](grpc::ClientContext *) { return; };
    EXPECT_THROW(GrpcRetry::retry(lambda, "", retryLimit, retryDelay, f,
                                  otherExceptions),
                 std::runtime_error);
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
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_NO_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay));

    failures = 0;
    EXPECT_NO_THROW(
        GrpcRetry::retry(lambda, "lambda()", retryLimit, retryDelay));
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
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay),
                 std::runtime_error);

    failures = 0;
    EXPECT_THROW(GrpcRetry::retry(lambda, "lambda()", retryLimit, retryDelay),
                 std::runtime_error);
}

TEST(GrpcRetry, AttachMetadata)
{
    buildboxcommon::RequestMetadataGenerator metadata_generator(
        "testing tool name", "v0.1");
    metadata_generator.set_action_id("action1");

    // Automatic success, no need to retry.
    auto grpc_invocation = [&](grpc::ClientContext &) {
        return grpc::Status::OK;
    };

    int attacher_calls = 0;
    auto metadata_attacher = [&](grpc::ClientContext *context) {
        metadata_generator.attach_request_metadata(context);
        attacher_calls++;
    };

    EXPECT_NO_THROW(
        GrpcRetry::retry(grpc_invocation, 0, 0, metadata_attacher));
    ASSERT_EQ(attacher_calls, 1);

    EXPECT_NO_THROW(GrpcRetry::retry(grpc_invocation, "grpc_invocation()", 0,
                                     0, metadata_attacher));
    ASSERT_EQ(attacher_calls, 2);
}
