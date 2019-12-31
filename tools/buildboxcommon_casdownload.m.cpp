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

#include <buildboxcommon_client.h>

#include <chrono>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using buildboxcommon::Digest;

/**
 * This utility stages a path using the RemoteCAS protocol.
 * Example usage:
 * casdownload "dev"
 * "http://distributedbuild-cas-dev-ob.bdns.bloomberg.com:60051"
 * "90bae5d80acda333c4b22317a23bc3ca6174e023d81d811333073ba048941c2a/980"
 * some_dir
 */

void printUsage(const char *program_name)
{
    std::cout << "Usage: " << program_name
              << " INSTANCE_NAME CASD_SERVER_ADDRESS ROOT_DIRECTORY_DIGEST "
                 "STAGE_DIRECTORY "
              << std::endl;
}

bool digestFromString(const std::string &s, Digest *digest)
{
    // "[hash in hex notation]/[size_bytes]"

    static const std::regex regex("([0-9a-fA-F]+)/(\\d+)");
    std::smatch matches;
    if (std::regex_search(s, matches, regex) && matches.size() == 3) {
        const std::string hash = matches[1];
        const std::string size = matches[2];

        digest->set_hash(hash);
        digest->set_size_bytes(std::stoll(size));
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    // Parsing arguments:
    if (argc < 5) {
        std::cerr << "Error: missing arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    const std::string casd_server_address = std::string(argv[2]);
    Digest rootDirectoryDigest;
    if (!digestFromString(argv[3], &rootDirectoryDigest)) {
        std::cerr << "Error: invalid digest \"" << argv[3] << "\""
                  << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    const std::string downloadDirectory = std::string(argv[4]);

    // connect to remote CAS server
    std::cout << "CAS client connecting to " << casd_server_address
              << std::endl;
    buildboxcommon::Client remoteCasClient;
    buildboxcommon::ConnectionOptions connection_options;
    connection_options.d_url = casd_server_address.c_str();
    connection_options.d_instanceName = argv[1];
    remoteCasClient.init(connection_options);

    const int fd =
        openat(AT_FDCWD, downloadDirectory.c_str(), O_DIRECTORY | O_RDONLY);
    if (fd < 0) {
        std::cout << "error opening \"" << downloadDirectory << "\", errno=["
                  << errno << ":" << strerror(errno) << "]" << std::endl;
        return 1;
    }

    std::cout << "Starting to download " << rootDirectoryDigest << " to \""
              << downloadDirectory << "\"" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    remoteCasClient.downloadDirectory(rootDirectoryDigest, downloadDirectory);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Finished downloading " << rootDirectoryDigest << " to \""
              << downloadDirectory << "\" in " << std::fixed
              << std::setprecision(3) << elapsed.count() << " second(s)"
              << std::endl;
    return 0;
}
