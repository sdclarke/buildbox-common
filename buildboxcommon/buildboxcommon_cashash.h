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

#ifndef INCLUDED_BUILDBOXCOMMON_CASHASH
#define INCLUDED_BUILDBOXCOMMON_CASHASH

#include <buildboxcommon_protos.h>

#include <iomanip>
#include <openssl/evp.h>
#include <set>

#include <sstream>
#include <string>

namespace buildboxcommon {

class DigestGenerator;

class CASHash {
  public:
    /**
     * Return a Digest corresponding to the contents of the given file
     * descriptor.
     */
    static Digest hash(int fd);

    /**
     * Return a Digest corresponding to the given string.
     */
    static Digest hash(const std::string &str);

    /**
     * Return a Digest corresponding to the contents of the file in `path`.
     *
     */
    static Digest hashFile(const std::string &path);

    /**
     * Return a `DigestFunction` message specifying the hash function used.
     */
    static DigestFunction_Value digestFunction();

  private:
    /**
     * For backwards compatibility, `CASHash` uses SHA256.
     */
    static const DigestFunction_Value s_digestFunctionValue =
        DigestFunction_Value_SHA256;
};

class DigestGenerator {
    /**
     * This class allows to generate `Digest` messages from blobs using
     * different digest functions.
     *
     * It employs the `EVP_` family of routines provided by OpenSSL.
     * (https://openssl.org/docs/man1.0.2/man3/EVP_DigestInit.html)
     */

  public:
    explicit DigestGenerator(
        DigestFunction_Value digest_function =
            DigestFunction_Value::DigestFunction_Value_SHA256);

    Digest hash(const std::string &data) const;
    Digest hash(int fd) const;

    inline DigestFunction_Value digest_function() const
    {
        return d_digestFunction;
    }

    static inline const std::set<DigestFunction_Value> &
    supportedDigestFunctions()
    {
        return s_supportedDigestFunctions;
    }

  private:
    const DigestFunction_Value d_digestFunction;
    const EVP_MD *d_digestFunctionStruct;

    static const std::set<DigestFunction_Value> s_supportedDigestFunctions;

  protected:
    // Size of the buffer used to read files from disk. Determines the
    // number of bytes that will be read in each chunk.
    static const size_t HASH_BUFFER_SIZE_BYTES;

  private:
    // Helpers to initialize and manipulate OpenSSL structures.
    // On errors they throw `std::runtime_error` exceptions.

    // Create and initialize an OpenSSL digest context to be used during a
    // call to `hash()`.
    // The context needs to be freed after use, so by storing it in an
    // `unique_ptr` we ensure it is destroyed automatically even if we throw.
    static void deleteDigestContext(EVP_MD_CTX *context);

    typedef std::unique_ptr<EVP_MD_CTX, decltype(&deleteDigestContext)>
        EVP_MD_CTX_ptr;
    EVP_MD_CTX_ptr createDigestContext() const;

    // Return the OpenSSL structure that represents the digest function to
    // use.
    static const EVP_MD *
    getDigestFunctionStruct(DigestFunction_Value digest_function_value);

    // Finish calculating a digest and generate the result.
    static Digest makeDigest(EVP_MD_CTX *digest_context,
                             const size_t data_size);

    // Calculate the hash of a portion of a file. This allows to read a file
    // from disk in chunks to avoid storing it wholly in memory.
    static void digestUpdate(const EVP_MD_CTX_ptr &digest_context,
                             const char *data, size_t size);

    // Take a hash value produced by OpenSSL and return a string with its
    // representation in hexadecimal.
    static std::string hashToHex(const unsigned char *hash_buffer,
                                 unsigned int hash_size);

    // If `status_code` is 0, throw an `std::runtime_error` exception with a
    // description containing `function_name`. Otherwise, do
    // nothing.
    // (Note that this considers 0 an error, following the OpenSSL convention.)
    static void throwIfNotSuccessful(int status_code,
                                     const std::string &function_name);

    // Helper to read a file in chunks and for each of them invoke an
    // update function.
    typedef std::function<void(char *, size_t)> IncrementalUpdateFunction;

    // Read a file in chunks and calculate its hash incrementally.
    static size_t
    processFile(int fd, const IncrementalUpdateFunction &update_function);
};

} // namespace buildboxcommon

#endif
