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
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_localcasstageddirectory.h>
#include <buildboxcommon_logging.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <sstream>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

namespace buildboxcommon {

static const int BUILDBOXCOMMON_RUNNER_USAGE_PAD_WIDTH = 32;
static const int BUILDBOXCOMMON_RUNNER_MAX_INLINED_OUTPUT = 1024;

namespace {
static void markNonBlocking(int fd)
{
    const int flags = fcntl(fd, F_GETFL);
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
        const auto bytes_written = write(fd, buffer, static_cast<size_t>(len));
        if (bytes_written == -1) {
            throw std::system_error(errno, std::system_category());
        }
        len -= bytes_written;
        buffer += bytes_written;
    }
}

static void usage(const char *name)
{
    std::clog << "\nusage: " << name << " [OPTIONS]\n";
    std::clog
        << "    --action=PATH               Path to read input Action from\n";
    std::clog << "    --action-result=PATH        Path to write output "
                 "ActionResult to\n";
    std::clog << "    --log-level=LEVEL           (default: info) Log "
                 "verbosity: "
              << buildboxcommon::logging::stringifyLogLevels() << "\n";
    std::clog << "    --verbose                   Set log level to debug\n";
    std::clog << "    --log-file=FILE             File to write log to\n";
    std::clog
        << "    --use-localcas              Use LocalCAS protocol methods\n";
    std::clog << "    --workspace-path=PATH           Location on disk which "
                 "runner will use as root when executing jobs\n";
    ConnectionOptions::printArgHelp(BUILDBOXCOMMON_RUNNER_USAGE_PAD_WIDTH);
}

} // namespace

volatile sig_atomic_t Runner::d_signal_status = 0;
void Runner::handleSignal(int signal) { d_signal_status = signal; }
sig_atomic_t Runner::getSignalStatus() { return d_signal_status; }

void Runner::registerSignals() const
{
    // Handle SIGINT, SIGTERM
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        BUILDBOX_LOG_ERROR("Unable to register signal handler for SIGINT");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        BUILDBOX_LOG_ERROR("Unable to register signal handler for SIGTERM");
        exit(1);
    }
}

Action Runner::readAction(const std::string &path) const
{
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        BUILDBOX_LOG_ERROR("Could not open Action file " << path << ": "
                                                         << strerror(errno));
        perror("buildbox-run input");
        exit(1);
    }

    Action input;
    const bool successful_read = input.ParseFromFileDescriptor(fd);
    close(fd);

    if (successful_read) {
        return input;
    }

    BUILDBOX_LOG_ERROR("Failed to parse Action from " << path);
    exit(1);
}

void Runner::initializeCasClient() const
{
    BUILDBOX_LOG_DEBUG("Initializing CAS " << this->d_casRemote.d_url);
    try {
        this->d_casClient->init(this->d_casRemote);
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_LOG_ERROR("Error initializing CAS client: " << e.what());
        exit(1);
    }
}

void Runner::writeActionResult(const ActionResult &action_result,
                               const std::string &path) const
{
    const int fd = open(path.c_str(), O_WRONLY);
    if (fd == -1) {
        BUILDBOX_LOG_ERROR("Could not save ActionResult to "
                           << path << ": " << strerror(errno));
        perror("buildbox-run output");
        exit(1);
    }

    const bool successful_write = action_result.SerializeToFileDescriptor(fd);
    close(fd);

    if (!successful_write) {
        BUILDBOX_LOG_ERROR(
            "Failed to serialize ActionResult before writing to " << path);
        exit(1);
    }
}

int Runner::main(int argc, char *argv[])
{
    if (!this->parseArguments(argc, argv)) {
        usage(argv[0]);
        printSpecialUsage();
        return 1;
    }

    Action input = readAction(this->d_inputPath);
    registerSignals();
    initializeCasClient();

    ActionResult result;
    try {
        BUILDBOX_LOG_DEBUG("Fetching " << input.command_digest().hash());
        const Command command =
            this->d_casClient->fetchMessage<Command>(input.command_digest());

        const auto signal_status = getSignalStatus();
        if (signal_status) {
            // If signal is set here, then no clean up necessary, return.
            return signal_status;
        }

        BUILDBOX_LOG_DEBUG("Executing command");
        result = this->execute(command, input.input_root_digest());
    }
    catch (const std::exception &e) {
        BUILDBOX_LOG_ERROR("Error executing command: " << e.what());

        result.set_exit_code(255);
        const std::string error_message =
            "buildbox-run: " + std::string(e.what()) + "\n";
        *(result.mutable_stderr_raw()) += error_message;
    }

    if (!this->d_outputPath.empty()) {
        writeActionResult(result, this->d_outputPath);
    }

    // At this point, if a signal is thrown, then `execute()` has happened
    // successfully and the results have been written.
    if (getSignalStatus()) {
        FileUtils::delete_directory(this->d_outputPath.c_str());
        return getSignalStatus();
    }

    return result.exit_code();
}

std::unique_ptr<StagedDirectory> Runner::stage(const Digest &digest,
                                               const std::string &stage_path,
                                               bool use_localcas_protocol)
{
    try {
        if (use_localcas_protocol) {
            return std::make_unique<LocalCasStagedDirectory>(
                digest, stage_path, this->d_casClient);
        }
        else {
            return std::make_unique<FallbackStagedDirectory>(
                digest, stage_path, this->d_casClient);
        }
    }
    catch (const std::exception &e) {
        const auto staging_mechanism = use_localcas_protocol
                                           ? "LocalCasStagedDirectory"
                                           : "FallbackStagedDirectory";
        BUILDBOX_LOG_DEBUG("Could not stage directory with digest \""
                           << digest << "\" using `" << staging_mechanism
                           << "`: " << e.what());
        throw;
    }
}

std::unique_ptr<StagedDirectory> Runner::stage(const Digest &digest,
                                               bool use_localcas_protocol)
{
    return stage(digest, "", use_localcas_protocol);
}

std::array<int, 2> Runner::createPipe() const
{
    std::array<int, 2> pipe_fds = {0, 0};

    if (pipe(pipe_fds.data()) == -1) {
        throw std::system_error(errno, std::system_category());
    }

    markNonBlocking(pipe_fds[0]);
    return pipe_fds;
}

void Runner::executeAndStore(const std::vector<std::string> &command,
                             ActionResult *result)
{
    std::ostringstream logline;
    for (const auto &token : command) {
        logline << token << " ";
    }

    BUILDBOX_LOG_DEBUG("Executing command: " << logline.str());
    const auto argc = command.size();

    std::unique_ptr<const char *[]> argv(new const char *[argc + 1]);
    for (unsigned int i = 0; i < argc; ++i) {
        argv[i] = command[i].c_str();
    }
    argv[argc] = nullptr;

    // Create pipes for stdout and stderr
    auto stdout_pipe = createPipe();
    auto stderr_pipe = createPipe();

    // Fork and exec
    const auto pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category());
    }
    else if (pid == 0) {
        // runs only on the child
        close(stdout_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[1]);

        close(stderr_pipe[0]);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stderr_pipe[1]);

        const int exec_status =
            execvp(argv[0], const_cast<char *const *>(argv.get()));

        // The lines below will only be executed if `execvp()` failed.
        int exit_code = 1;
        if (exec_status != 0) {
            const auto exec_error = errno;
            BUILDBOX_LOG_ERROR("Error while calling `execvp("
                               << logline.str()
                               << ")`: " << strerror(exec_error));

            // Following the Bash convention for exit codes.
            // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
            if (exec_error == ENOENT) {
                exit_code = 127; // "command not found"
            }
            else {
                exit_code = 126; // Command invoked cannot execute
            }
        }

        perror(argv[0]);
        _Exit(exit_code);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    fd_set fds_to_read;
    FD_ZERO(&fds_to_read);
    FD_SET(stdout_pipe[0], &fds_to_read);
    FD_SET(stderr_pipe[0], &fds_to_read);

    char buffer[4096];
    while (FD_ISSET(stdout_pipe[0], &fds_to_read) ||
           FD_ISSET(stderr_pipe[0], &fds_to_read)) {

        fd_set fdsSuccessfullyRead = fds_to_read;
        select(FD_SETSIZE, &fdsSuccessfullyRead, nullptr, nullptr, nullptr);

        if (FD_ISSET(stdout_pipe[0], &fdsSuccessfullyRead)) {
            const auto bytesRead =
                read(stdout_pipe[0], buffer, sizeof(buffer));

            if (bytesRead > 0) {
                writeAll(STDOUT_FILENO, buffer, bytesRead);
                // TODO: do we wanna try and handle stdout/stderr that's too
                // big to fit in memory?
                *(result->mutable_stdout_raw()) +=
                    std::string(buffer, bytesRead);
            }
            else if (!(bytesRead == -1 &&
                       (errno == EINTR || errno == EAGAIN))) {
                FD_CLR(stdout_pipe[0], &fds_to_read);
            }
        }

        if (FD_ISSET(stderr_pipe[0], &fdsSuccessfullyRead)) {
            const auto bytesRead =
                read(stderr_pipe[0], buffer, sizeof(buffer));

            if (bytesRead > 0) {
                writeAll(STDERR_FILENO, buffer, bytesRead);
                // TODO: do we wanna try and handle stdout/stderr that's too
                // big to fit in memory?
                *(result->mutable_stderr_raw()) +=
                    std::string(buffer, bytesRead);
            }
            else if (!(bytesRead == -1 &&
                       (errno == EINTR || errno == EAGAIN))) {
                FD_CLR(stderr_pipe[0], &fds_to_read);
            }
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    BUILDBOX_LOG_DEBUG("Finished reading");

    uploadIfNeeded(result->mutable_stdout_raw(),
                   result->mutable_stdout_digest());
    uploadIfNeeded(result->mutable_stderr_raw(),
                   result->mutable_stderr_digest());

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::system_error(errno, std::system_category());
    }

    if (WIFEXITED(status)) {
        result->set_exit_code(WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
        result->set_exit_code(128 + WTERMSIG(status));
        // Exit code as returned by Bash.
        // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
    }
    else {
        /* According to the documentation for `waitpid()` we should never get
         * here:
         *
         * "If the information pointed to by stat_loc was stored by a call to
         * waitpid() that did not specify the WUNTRACED  or
         * CONTINUED flags, or by a call to the wait() function,
         * exactly one of the macros WIFEXITED(*stat_loc) and
         * WIFSIGNALED(*stat_loc) shall evaluate to a non-zero value."
         *
         * (https://pubs.opengroup.org/onlinepubs/009695399/functions/wait.html)
         */
        throw std::runtime_error(
            "`waitpid()` returned an unexpected status: " +
            std::to_string(status));
    }
}

bool Runner::parseArguments(int argc, char *argv[])
{
    argv++;
    argc--;

    while (argc > 0) {
        const char *arg = argv[0];
        const char *assign = strchr(arg, '=');
        if (this->parseArg(arg)) {
            // Argument was handled by a subclass's parseArg method.
        }
        else if (this->d_casRemote.parseArg(arg)) {
            // Argument was handled by ConnectionOptions.
        }
        else if (arg[0] == '-' && arg[1] == '-') {
            arg += 2;
            if (assign) {
                const auto key_len = static_cast<size_t>(assign - arg);
                const char *value = assign + 1;
                if (strncmp(arg, "action", key_len) == 0) {
                    this->d_inputPath = std::string(value);
                }
                else if (strncmp(arg, "action-result", key_len) == 0) {
                    this->d_outputPath = std::string(value);
                }
                else if (strncmp(arg, "workspace-path", key_len) == 0) {
                    this->d_stage_path = std::string(value);
                }
                else if (strncmp(arg, "log-level", key_len) == 0) {
                    std::string level(value);
                    std::transform(level.begin(), level.end(), level.begin(),
                                   ::tolower);
                    if (logging::stringToLogLevel.find(level) ==
                        logging::stringToLogLevel.end()) {
                        std::cerr << "Invalid log level." << std::endl;
                        return false;
                    }
                    BUILDBOX_LOG_SET_LEVEL(
                        logging::stringToLogLevel.at(level));
                }
                else if (strncmp(arg, "log-file", key_len) == 0) {
                    FILE *fp = fopen(value, "w");
                    if (fp == nullptr) {
                        std::cerr << "--log-file: unable to write to "
                                  << std::string(value) << std::endl;
                        return false;
                    }
                    fclose(fp);
                    BUILDBOX_LOG_SET_FILE(value);
                }
                else {
                    std::cerr << "Invalid option " << argv[0] << std::endl;
                    return false;
                }
            }
            else {
                if (strcmp(arg, "help") == 0) {
                    return false;
                }
                else if (strcmp(arg, "use-localcas") == 0) {
                    this->d_use_localcas_protocol = true;
                }
                else if (strcmp(arg, "verbose") == 0) {
                    BUILDBOX_LOG_SET_LEVEL(LogLevel::DEBUG);
                }
                else {
                    std::cerr << "Invalid option " << argv[0] << std::endl;
                    return false;
                }
            }
        }
        else {
            std::cerr << "Unexpected argument " << arg << std::endl;
            return false;
        }
        argv++;
        argc--;
    }

    if (!this->d_casRemote.d_url) {
        std::cerr << "CAS server URL is missing." << std::endl;
        return false;
    }
    return true;
}

void Runner::uploadIfNeeded(std::string *str, Digest *digest) const
{
    if (str->length() > BUILDBOXCOMMON_RUNNER_MAX_INLINED_OUTPUT) {
        BUILDBOX_LOG_DEBUG("Uploading " << digest->hash());
        *digest = CASHash::hash(*str);

        try {
            this->d_casClient->upload(*str, *digest);
            BUILDBOX_LOG_DEBUG("Done uploading " << digest->hash());
        }
        catch (const std::runtime_error &e) {
            BUILDBOX_LOG_ERROR(
                "Could not upload stdout/stderr contents, output lost: "
                << e.what());
        }

        str->clear();
    }
}

} // namespace buildboxcommon
