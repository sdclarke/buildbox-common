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

#include <buildboxcommon_runner.h>

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_fallbackstageddirectory.h>

#include <cerrno>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

#define BUILDBOXCOMMON_RUNNER_MAX_INLINED_OUTPUT (1024)

namespace {
static void markNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        throw std::system_error(errno, std::system_category());
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::system_error(errno, std::system_category());
    }
}

static void writeAll(int fd, const char *buffer, ssize_t len)
{
    while (len > 0) {
        const auto bytesWritten = write(fd, buffer, len);
        if (bytesWritten == -1) {
            throw std::system_error(errno, std::system_category());
        }
        len -= bytesWritten;
        buffer += bytesWritten;
    }
}

static void usage(const char *name)
{
    std::cerr << "usage: " << name << " [OPTIONS]\n";
    std::cerr
        << "    --action=PATH               Path to read input Action from\n";
    std::cerr << "    --action-result=PATH        Path to write output "
                 "ActionResult to\n";
    ConnectionOptions::printArgHelp(32);
}

} // namespace

int Runner::main(int argc, char *argv[])
{
    if (!this->parseArguments(argc, argv)) {
        usage(argv[0]);
        return 1;
    }
    this->d_casClient->init(this->d_casRemote);

    const int inFd = open(this->d_inputPath, O_RDONLY);
    if (inFd == -1) {
        perror("buildbox-run input");
        return 1;
    }
    Action input;
    if (!input.ParseFromFileDescriptor(inFd)) {
        std::cerr << "buildbox-run: failed to parse Action\n";
        return 1;
    }
    close(inFd);

    ActionResult result;
    try {
        const Command command =
            this->d_casClient->fetchMessage<Command>(input.command_digest());
        result = this->execute(command, input.input_root_digest());
    }
    catch (const std::exception &e) {
        result.set_exit_code(255);
        std::string msg =
            std::string("buildbox-run: ") + e.what() + std::string("\n");
        std::cerr << msg;
        *(result.mutable_stderr_raw()) += msg;
    }

    if (this->d_outputPath != nullptr && this->d_outputPath[0] != '\0') {
        int outFd = open(this->d_outputPath, O_WRONLY);
        if (outFd == -1) {
            perror("buildbox-run output");
            return 1;
        }
        if (!result.SerializeToFileDescriptor(outFd)) {
            std::cerr << "buildbox-run: failed to serialize ActionResult\n";
            return 1;
        }
        close(outFd);
    }

    return result.exit_code();
}

std::unique_ptr<StagedDirectory> Runner::stage(const Digest &digest)
{
    // TODO use the LocalCAS protocol when available.
    return std::unique_ptr<StagedDirectory>(
        new FallbackStagedDirectory(digest, this->d_casClient));
}

void Runner::executeAndStore(std::vector<std::string> command,
                             ActionResult *result)
{
    int argc = command.size();
    const char *argv[argc + 1];
    for (int i = 0; i < argc; ++i) {
        argv[i] = command[i].c_str();
    }
    argv[argc] = nullptr;

    // Create pipes for stdout and stderr
    int stdoutPipeFds[2] = {0};
    if (pipe(stdoutPipeFds) == -1) {
        throw std::system_error(errno, std::system_category());
    }
    markNonBlocking(stdoutPipeFds[0]);
    int stderrPipeFds[2] = {0};
    if (pipe(stderrPipeFds) == -1) {
        throw std::system_error(errno, std::system_category());
    }
    markNonBlocking(stderrPipeFds[0]);

    // Fork and exec
    const auto pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category());
    }
    else if (pid == 0) {
        // runs only on the child
        close(stdoutPipeFds[0]);
        dup2(stdoutPipeFds[1], STDOUT_FILENO);
        close(stdoutPipeFds[1]);

        close(stderrPipeFds[0]);
        dup2(stderrPipeFds[1], STDERR_FILENO);
        close(stderrPipeFds[1]);

        execvp(argv[0], const_cast<char *const *>(argv));
        perror(argv[0]);
        _Exit(1);
    }
    close(stdoutPipeFds[1]);
    close(stderrPipeFds[1]);

    fd_set fdsToRead;
    FD_ZERO(&fdsToRead);
    FD_SET(stdoutPipeFds[0], &fdsToRead);
    FD_SET(stderrPipeFds[0], &fdsToRead);

    char buffer[4096];
    while (FD_ISSET(stdoutPipeFds[0], &fdsToRead) ||
           FD_ISSET(stderrPipeFds[0], &fdsToRead)) {
        fd_set fdsSuccessfullyRead = fdsToRead;
        select(FD_SETSIZE, &fdsSuccessfullyRead, nullptr, nullptr, nullptr);
        if (FD_ISSET(stdoutPipeFds[0], &fdsSuccessfullyRead)) {
            const auto bytesRead =
                read(stdoutPipeFds[0], buffer, sizeof(buffer));
            if (bytesRead > 0) {
                writeAll(STDOUT_FILENO, buffer, bytesRead);
                // TODO: do we wanna try and handle stdout/stderr that's too
                // big to fit in memory?
                *(result->mutable_stdout_raw()) +=
                    std::string(buffer, bytesRead);
            }
            else if (!(bytesRead == -1 &&
                       (errno == EINTR || errno == EAGAIN))) {
                FD_CLR(stdoutPipeFds[0], &fdsToRead);
            }
        }
        if (FD_ISSET(stderrPipeFds[0], &fdsSuccessfullyRead)) {
            const auto bytesRead =
                read(stderrPipeFds[0], buffer, sizeof(buffer));
            if (bytesRead > 0) {
                writeAll(STDERR_FILENO, buffer, bytesRead);
                // TODO: do we wanna try and handle stdout/stderr that's too
                // big to fit in memory?
                *(result->mutable_stderr_raw()) +=
                    std::string(buffer, bytesRead);
            }
            else if (!(bytesRead == -1 &&
                       (errno == EINTR || errno == EAGAIN))) {
                FD_CLR(stderrPipeFds[0], &fdsToRead);
            }
        }
    }
    close(stdoutPipeFds[0]);
    close(stderrPipeFds[0]);

    uploadIfNeeded(result->mutable_stdout_raw(),
                   result->mutable_stdout_digest());
    uploadIfNeeded(result->mutable_stderr_raw(),
                   result->mutable_stderr_digest());

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::system_error(errno, std::system_category());
    }
    result->set_exit_code(WEXITSTATUS(status));
}

bool Runner::parseArguments(int argc, char *argv[])
{
    char *prgname = argv[0];
    argv++;
    argc--;

    while (argc > 0) {
        const char *arg = argv[0];
        const char *assign = strchr(arg, '=');
        if (this->d_casRemote.parseArg(arg)) {
            // Argument was handled by ConnectionOptions.
        }
        else if (arg[0] == '-' && arg[1] == '-') {
            arg += 2;
            if (assign) {
                int key_len = assign - arg;
                const char *value = assign + 1;
                if (strncmp(arg, "action", key_len) == 0) {
                    this->d_inputPath = value;
                }
                else if (strncmp(arg, "action-result", key_len) == 0) {
                    this->d_outputPath = value;
                }
                else {
                    std::cerr << "Invalid option " << argv[0] << "\n";
                    return false;
                }
            }
            else {
                if (strcmp(arg, "help") == 0) {
                    return false;
                }
                else {
                    std::cerr << "Invalid option " << argv[0] << "\n";
                    return false;
                }
            }
        }
        else {
            std::cerr << "Unexpected argument " << arg << "\n";
            return false;
        }
        argv++;
        argc--;
    }

    if (!this->d_casRemote.d_url) {
        std::cerr << "CAS server URL is missing\n";
        return false;
    }
    return true;
}

void Runner::uploadIfNeeded(std::string *str, Digest *digest)
{
    if (str->length() > BUILDBOXCOMMON_RUNNER_MAX_INLINED_OUTPUT) {
        *digest = CASHash::hash(*str);
        this->d_casClient->upload(*str, *digest);
        str->clear();
    }
}

} // namespace buildboxcommon
