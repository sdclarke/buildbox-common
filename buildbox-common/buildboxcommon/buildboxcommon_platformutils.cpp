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

#include <buildboxcommon_platformutils.h>

#include <buildboxcommon_exception.h>

#include <algorithm>
#include <cerrno>
#include <string>
#include <system_error>

#include <sys/utsname.h>

using namespace buildboxcommon;

std::string PlatformUtils::getHostOSFamily()
{
#if defined(__APPLE__)
    return "macos";
#else
    struct utsname buf;
    if (uname(&buf) < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::generic_category, "uname failed");
    }

    std::string osfamily(buf.sysname);

    // Canonicalize to lower case
    std::transform(osfamily.begin(), osfamily.end(), osfamily.begin(),
                   ::tolower);
    return osfamily;
#endif
}

std::string PlatformUtils::getHostISA()
{
#if defined(_AIX) && defined(__powerpc__)
    // AIX doesn't return ISA in utsname.machine
#if defined(__BIG_ENDIAN__)
    return "power-isa-be";
#else
    return "power-isa-le";
#endif
#else
    struct utsname buf;
    if (uname(&buf) < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::generic_category, "uname failed");
    }

    std::string arch(buf.machine);

    if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
        arch = "x86-32";
    }
    else if (arch == "amd64" || arch == "x86_64") {
        arch = "x86-64";
    }
    else if (arch == "arm") {
        arch = "aarch32";
    }
    else if (arch == "armv8l") {
        arch = "aarch64";
    }
    else if (arch == "armv8b") {
        arch = "aarch64-be";
    }
    else if (arch == "ppc64") {
        arch = "power-isa-be";
    }
    else if (arch == "ppc64le") {
        arch = "power-isa-le";
    }
    else if (arch == "sparc" || arch == "sparc64" || arch == "sun4v") {
        arch = "sparc-v9";
    }

    return arch;
#endif
}
