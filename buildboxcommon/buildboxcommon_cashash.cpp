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

#include <buildboxcommon_cashash.h>

#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <unistd.h>

namespace buildboxcommon {

#define BUILDBOXCOMMON_HASH_BUFFER_SIZE (1024 * 1024)

namespace {
static std::string hashToHex(unsigned char hash[])
{
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
} // namespace

Digest CASHash::hash(int fd)
{
    lseek(fd, 0, SEEK_SET);
    std::vector<char> buffer(BUILDBOXCOMMON_HASH_BUFFER_SIZE);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;
    while ((bytesRead = read(fd, &buffer[0], buffer.size())) > 0) {
        SHA256_Update(&sha256, &buffer[0], bytesRead);
        totalBytesRead += bytesRead;
    }
    SHA256_Final(hash, &sha256);

    Digest result;
    result.set_hash(hashToHex(hash));
    result.set_size_bytes(totalBytesRead);

    return result;
}

Digest CASHash::hash(const std::string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, &str[0], str.size());
    SHA256_Final(hash, &sha256);

    Digest result;
    result.set_hash(hashToHex(hash));
    result.set_size_bytes(str.size());

    return result;
}

} // namespace buildboxcommon
