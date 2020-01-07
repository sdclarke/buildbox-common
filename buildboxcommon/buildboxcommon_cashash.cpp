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

#include <buildboxcommon_cashash.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_logging.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace buildboxcommon {

const size_t DigestGenerator::HASH_BUFFER_SIZE_BYTES = (1024 * 64);

const std::set<DigestFunction_Value>
    DigestGenerator::s_supportedDigestFunctions = {
        DigestFunction_Value_MD5, DigestFunction_Value_SHA1,
        DigestFunction_Value_SHA256, DigestFunction_Value_SHA384,
        DigestFunction_Value_SHA512};

#ifndef BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE
#error "Digest function not defined"
#else
const DigestFunction_Value CASHash::s_digestFunctionValue =
    static_cast<DigestFunction_Value>(BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE);
#endif

/**
 * For backwards compatibility, `CASHash` encapsulates an instance of the newer
 * `DigestGenerator` class.
 */

Digest CASHash::hash(int fd)
{
    return DigestGenerator(s_digestFunctionValue).hash(fd);
}

Digest CASHash::hash(const std::string &str)
{
    return DigestGenerator(s_digestFunctionValue).hash(str);
}

Digest CASHash::hashFile(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening file \"" << path << "\"");
    }

    try {
        const Digest d = CASHash::hash(fd);
        close(fd);
        return d;
    }
    catch (...) {
        close(fd);
        throw;
    }
}

DigestFunction_Value CASHash::digestFunction()
{
    return s_digestFunctionValue;
}

DigestGenerator::DigestGenerator(DigestFunction_Value digest_function)
    : d_digestFunction(digest_function),
      d_digestFunctionStruct(getDigestFunctionStruct(digest_function))
{
    // If an invalid function is given, `getDigestFunctionStruct()` will
    // throw.
}

Digest DigestGenerator::hash(const std::string &data) const
{
    auto digest_context = createDigestContext();
    digestUpdate(digest_context, data.c_str(), data.size());
    return makeDigest(digest_context.get(), data.size());
}

Digest DigestGenerator::hash(int fd) const
{
    auto digest_context = createDigestContext();

    // Reading file in chunks and computing the hash incrementally:
    const auto update_function = [&digest_context](const char *buffer,
                                                   size_t data_size) {
        digestUpdate(digest_context, buffer, data_size);
    };
    const size_t bytes_read = processFile(fd, update_function);

    // Generating hash string:
    return makeDigest(digest_context.get(), bytes_read);
}

const EVP_MD *DigestGenerator::getDigestFunctionStruct(
    DigestFunction_Value digest_function_value)
{
    switch (digest_function_value) {
        case DigestFunction_Value_MD5:
            return EVP_md5();
        case DigestFunction_Value_SHA1:
            return EVP_sha1();
        case DigestFunction_Value_SHA256:
            return EVP_sha256();
        case DigestFunction_Value_SHA384:
            return EVP_sha384();
        case DigestFunction_Value_SHA512:
            return EVP_sha512();

        default:
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error, "Digest function value not supported: "
                                        << digest_function_value);
    }
}

void DigestGenerator::digestUpdate(const EVP_MD_CTX_ptr &digest_context,
                                   const char *data, size_t data_size)
{
    throwIfNotSuccessful(
        EVP_DigestUpdate(digest_context.get(), data, data_size),
        "EVP_DigestUpdate()");
}

Digest DigestGenerator::makeDigest(EVP_MD_CTX *digest_context,
                                   const size_t data_size)
{
    unsigned char hash_buffer[EVP_MAX_MD_SIZE];

    unsigned int message_length;
    throwIfNotSuccessful(
        EVP_DigestFinal_ex(digest_context, hash_buffer, &message_length),
        "EVP_DigestFinal_ex()");

    const std::string hash = hashToHex(hash_buffer, message_length);

    Digest digest;
    digest.set_hash(hash);
    digest.set_size_bytes(static_cast<google::protobuf::int64>(data_size));
    return digest;
}

std::string DigestGenerator::hashToHex(const unsigned char *hash_buffer,
                                       unsigned int hash_size)
{
    std::ostringstream ss;
    for (unsigned int i = 0; i < hash_size; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash_buffer[i]);
    }
    return ss.str();
}

size_t
DigestGenerator::processFile(int fd,
                             const IncrementalUpdateFunction &update_function)
{
    std::array<char, HASH_BUFFER_SIZE_BYTES> buffer;
    size_t total_bytes_read = 0;

    lseek(fd, 0, SEEK_SET);

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer.data(), buffer.size())) > 0) {
        update_function(buffer.data(), static_cast<size_t>(bytes_read));
        total_bytes_read += static_cast<size_t>(bytes_read);
    }
    if (bytes_read == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error in read on file descriptor " << fd);
    }

    return total_bytes_read;
}

void DigestGenerator::throwIfNotSuccessful(int status_code,
                                           const std::string &function_name)
{
    if (status_code == 0) {
        throw std::runtime_error(function_name + " failed.");
    }
    // "EVP_DigestInit_ex(), EVP_DigestUpdate() and EVP_DigestFinal_ex() return
    // 1 for success and 0 for failure."
    // https://openssl.org/docs/man1.1.0/man3/EVP_DigestInit.html
}

DigestGenerator::EVP_MD_CTX_ptr DigestGenerator::createDigestContext() const
{
    EVP_MD_CTX_ptr digest_context(EVP_MD_CTX_create(),
                                  &DigestGenerator::deleteDigestContext);
    // `EVP_MD_CTX_ptr` is an alias for `unique_ptr`, it will make sure that
    // the context object is freed if something goes wrong and we need to
    // throw.

    if (!digest_context) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Error creating `EVP_MD_CTX` context struct");
    }

    throwIfNotSuccessful(EVP_DigestInit_ex(digest_context.get(),
                                           d_digestFunctionStruct, nullptr),
                         "EVP_DigestInit_ex()");

    return digest_context;
}

void DigestGenerator::deleteDigestContext(EVP_MD_CTX *context)
{
    EVP_MD_CTX_destroy(context);
    // ^ Calling this macro ensures compatibility with OpenSSL 1.0.2:
    // "EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were
    // renamed to EVP_MD_CTX_new() and EVP_MD_CTX_free() in OpenSSL 1.1."
    // (https://openssl.org/docs/man1.1.0/man3/EVP_DigestInit.html)
}

} // namespace buildboxcommon
