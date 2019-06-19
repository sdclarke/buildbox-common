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

#ifndef INCLUDED_BUILDBOXCOMMON_CASHASH
#define INCLUDED_BUILDBOXCOMMON_CASHASH

#include <buildboxcommon_protos.h>

#include <string>

namespace buildboxcommon {
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
     * Return a `DigestFunction` message specifying the hash function used.
     */
    static DigestFunction digestFunction() { return DigestFunction::SHA256; }
};
} // namespace buildboxcommon

#endif
