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
#include <buildboxcommon_exception.h>
#include <buildboxcommon_fallbackstageddirectory.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_localcasstageddirectory.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_systemutils.h>
#include <buildboxcommon_temporaryfile.h>
#include <buildboxcommon_timeutils.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <functional>
#include <stdlib.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace buildboxcommon {

static const int BUILDBOXCOMMON_RUNNER_USAGE_PAD_WIDTH = 32;

namespace {
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

    std::string programName(name); // Binary name without path components
    programName = programName.substr(
        std::max<size_t>(0, programName.find_last_of("/") + 1));
    std::clog << "    --log-directory=DIR         Write logs to this "
                 "directory with filenames:\n"
                 "                                "
              << programName
              << ".<hostname>.<user name>.log.<severity "
                 "level>.<date>.<time>.<pid>\n";

    std::clog
        << "    --use-localcas              Use LocalCAS protocol methods "
           "(default behavior)\n"
        << "                                NOTE: this option will be "
           "deprecated.\n";
    std::clog << "    --disable-localcas          Do not use LocalCAS "
                 "protocol methods\n";
    std::clog << "    --workspace-path=PATH       Location on disk which "
                 "runner will use as root when executing jobs\n";
    std::clog
        << "    --stdout-file=FILE          File to redirect the command's "
           "stdout to\n";
    std::clog
        << "    --stderr-file=FILE          File to redirect the command's "
           "stderr to\n";
    std::clog
        << "    --no-logs-capture           Do not capture and upload the "
           "contents written to stdout and stderr\n";
    std::clog << "    --capabilities              Print capabilities "
                 "supported by this runner\n";
    std::clog << "    --validate-parameters       Only check whether all the "
                 "required parameters are being passed and that no\n"
                 "                                unknown options are given. "
                 "Exits with a status code containing "
                 "the result (0 if successful).\n";
    ConnectionOptions::printArgHelp(BUILDBOXCOMMON_RUNNER_USAGE_PAD_WIDTH);
}

class Runner_StandardOutputFile final {
  public:
    /* If the specified path is empty, generate a temporary file and keep it
     * until this object goes out of scope. Otherwise just keep track of the
     * path (without any side effects on the filesystem).
     */
    explicit Runner_StandardOutputFile(const std::string &path)
    {
        if (path.empty()) {
            d_tmpFile = std::make_unique<buildboxcommon::TemporaryFile>();
            d_tmpFile->close();

            d_path = d_tmpFile->strname();
        }
        else {
            d_path = path;
        }
    }

    ~Runner_StandardOutputFile() {}

    const std::string &name() const { return d_path; }

  private:
    std::string d_path;
    std::unique_ptr<buildboxcommon::TemporaryFile> d_tmpFile;
};
} // namespace

volatile sig_atomic_t Runner::d_signal_status = 0;
void Runner::handleSignal(int signal) { d_signal_status = signal; }
sig_atomic_t Runner::getSignalStatus() { return d_signal_status; }

void Runner::recursively_chmod_directories(const char *path, mode_t mode)
{
    {
        DirentWrapper root(path);

        bool encountered_permission_errors = false;

        FileUtils::DirectoryTraversalFnPtr chmod_func =
            [&mode, &encountered_permission_errors](
                const char *dir_path = nullptr, int fd = 0) {
                if (fchmod(fd, mode) == -1) {
                    const int chmod_error = errno;
                    if (chmod_error == EPERM) {
                        // Logging every instance of this error might prove too
                        // noisy when staging using chroots. We aggregate them
                        // into a single warning message.
                        encountered_permission_errors = true;
                    }
                    else {
                        BUILDBOX_LOG_WARNING("Unable to chmod dir: "
                                             << dir_path << " errno: "
                                             << strerror(chmod_error));
                    }
                };
            };

        FileUtils::fileDescriptorTraverseAndApply(&root, chmod_func, nullptr,
                                                  true);

        if (encountered_permission_errors) {
            BUILDBOX_LOG_WARNING("Failed to `chmod()` some directories in \""
                                 << path
                                 << "\" due to permission issues (`EPERM`).");
        }
    }
}

void Runner::registerSignals() const
{
    // Handle SIGINT, SIGTERM
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        BUILDBOX_RUNNER_LOG(ERROR,
                            "Unable to register signal handler for SIGINT");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        BUILDBOX_RUNNER_LOG(ERROR,
                            "Unable to register signal handler for SIGTERM");
        exit(1);
    }
}

Action Runner::readAction(const std::string &path) const
{
    try {
        return ProtoUtils::readProtobufFromFile<Action>(path);
    }
    catch (const std::runtime_error &e) {
        std::ostringstream errorMessage;
        errorMessage << "Failed to read Action from [" << path
                     << "]: " << e.what();

        const auto errorMessageStr = errorMessage.str();
        BUILDBOX_RUNNER_LOG(ERROR, errorMessageStr);

        writeErrorStatusFile(grpc::StatusCode::INTERNAL, errorMessageStr);
    }

    exit(1);
}

void Runner::initializeCasClient() const
{
    BUILDBOX_RUNNER_LOG(DEBUG, "Initializing CAS client to connect to: \""
                                   << this->d_casRemote.d_url << "\"");
    try {
        this->d_casClient->init(this->d_casRemote);
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_RUNNER_LOG(ERROR,
                            "Error initializing CAS client: " << e.what());
        exit(1);
    }
}

void Runner::writeActionResult(const ActionResult &action_result,
                               const std::string &path) const
{
    try {
        ProtoUtils::writeProtobufToFile(action_result, path);
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_RUNNER_LOG(ERROR,
                            "Could not save ActionResult: " << e.what());
        exit(1);
    }
}

void Runner::writeErrorStatusFile(const google::protobuf::int32 errorCode,
                                  const std::string &errorMessage) const
{
    if (!d_outputPath.empty()) {
        google::rpc::Status status;
        status.set_code(errorCode);
        status.set_message(errorMessage);

        writeStatusFile(status, errorStatusCodeFilePath(d_outputPath));
    }
}

std::string
Runner::errorStatusCodeFilePath(const std::string &actionResultPath)
{
    if (actionResultPath.empty()) {
        return "";
    }
    return actionResultPath + ".error-status";
}

void Runner::writeStatusFile(const google::rpc::Status &status,
                             const std::string &path) const
{
    try {
        ProtoUtils::writeProtobufToFile(status, path);
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_RUNNER_LOG(ERROR,
                            "Could not save Status proto file: " << e.what());
    }
}

Command Runner::fetchCommand(const Digest &command_digest) const
{
    google::rpc::Status errorStatus;
    try {
        return this->d_casClient->fetchMessage<Command>(command_digest);
    }
    catch (const GrpcError &e) {
        errorStatus.set_code(e.status.error_code());
        errorStatus.set_message(e.status.error_message());
    }
    catch (const std::runtime_error &e) {
        errorStatus.set_code(grpc::StatusCode::INTERNAL);
        errorStatus.set_message(e.what());
    }

    std::ostringstream errorMessage;
    errorMessage << "Error fetching Command with digest \"" << command_digest
                 << "\" from \"" << d_casRemote.d_url
                 << "\": " << errorStatus.message();

    const std::string errorMessageStr = errorMessage.str();
    BUILDBOX_RUNNER_LOG(ERROR, errorMessageStr);

    writeErrorStatusFile(errorStatus.code(), errorMessageStr);
    exit(1);
}

int Runner::main(int argc, char *argv[])
{
    if (!this->parseArguments(argc, argv)) {
        usage(argv[0]);
        printSpecialUsage();
        return 1;
    }
    else if (this->d_validateParametersAndExit) {
        std::cerr << "Asked to only validate the CLI parameters "
                     "(--validate-parameters) and the check "
                     "suceeded: exiting 0. \n";
        return 0;
    }

    logging::Logger::getLoggerInstance().initialize(argv[0]);
    // (`parseArguments()` already set the destination of logs.)

    // now set the logging ver;bosity level after the init
    BUILDBOX_LOG_SET_LEVEL(this->d_logLevel);

    // -- Worker started --
    const auto worker_start_time = TimeUtils::now();

    const Action input = readAction(this->d_inputPath);
    this->d_action_digest = CASHash::hash(input.SerializeAsString());

    registerSignals();
    initializeCasClient();

    BUILDBOX_RUNNER_LOG(DEBUG, "Fetching Command " << input.command_digest());
    const Command command = fetchCommand(input.command_digest());

    ActionResult result;
    try {
        const auto signal_status = getSignalStatus();
        if (signal_status) {
            // If signal is set here, then no clean up necessary, return.
            return signal_status;
        }

        BUILDBOX_RUNNER_LOG(DEBUG, "Executing command");
        result = this->execute(command, input.input_root_digest());
    }
    catch (const std::exception &e) {
        BUILDBOX_RUNNER_LOG(ERROR, "Error executing command: " << e.what());

        writeErrorStatusFile(grpc::StatusCode::INTERNAL,
                             "execute() threw: " + std::string(e.what()));

        return EXIT_FAILURE;
    }
    //  -- Worker finished, set start/completed timestamps --
    auto *result_metadata = result.mutable_execution_metadata();
    result_metadata->mutable_worker_completed_timestamp()->CopyFrom(
        TimeUtils::now());
    result_metadata->mutable_worker_start_timestamp()->CopyFrom(
        worker_start_time);

    if (!this->d_outputPath.empty()) {
        writeActionResult(result, this->d_outputPath);
    }

    return getSignalStatus();
}

std::unique_ptr<StagedDirectory> Runner::stage(const Digest &digest,
                                               const std::string &stage_path,
                                               bool use_localcas_protocol)
{
    try {
        auto stagedDirectory = std::unique_ptr<StagedDirectory>();
        if (use_localcas_protocol) {
            stagedDirectory = std::make_unique<LocalCasStagedDirectory>(
                digest, stage_path, this->d_casClient);
        }
        else {
            stagedDirectory = std::make_unique<FallbackStagedDirectory>(
                digest, stage_path, this->d_casClient);
        }
        this->d_stage_path = stagedDirectory->getPath();
        return stagedDirectory;
    }
    catch (const std::exception &e) {
        const auto staging_mechanism = use_localcas_protocol
                                           ? "LocalCasStagedDirectory"
                                           : "FallbackStagedDirectory";
        BUILDBOX_RUNNER_LOG(DEBUG, "Could not stage directory with digest \""
                                       << digest << "\" using `"
                                       << staging_mechanism
                                       << "`: " << e.what());
        throw;
    }
}

std::unique_ptr<StagedDirectory> Runner::stage(const Digest &digest,
                                               bool use_localcas_protocol)
{
    return stage(digest, "", use_localcas_protocol);
}

void Runner::createOutputDirectories(const Command &command,
                                     const std::string &workingDir) const
{
    auto createDirectoryIfNeeded = [&](const std::string &output) {
        if (output.find("/") != std::string::npos) {
            std::string directory_location =
                workingDir + "/" + output.substr(0, output.rfind("/"));
            try {
                FileUtils::createDirectory(directory_location.c_str());
            }
            catch (const std::system_error &e) {
                BUILDBOX_RUNNER_LOG(ERROR, "Error while creating directory "
                                               << directory_location << " : "
                                               << e.what());
                throw;
            }
            BUILDBOX_RUNNER_LOG(DEBUG, "Created parent output directory: "
                                           << directory_location);
        }
    };

    // In v2.1 of the REAPI: "[output_paths] supersedes the DEPRECATED
    // `output_files` and `output_directories` fields. If `output_paths` is
    // used, `output_files` and `output_directories` will be ignored!"
    if (command.output_paths_size() > 0) {
        // The runner is still required to create the directories leading up to
        // the output paths
        std::for_each(command.output_paths().cbegin(),
                      command.output_paths().cend(), createDirectoryIfNeeded);
    }
    else {
        // Create parent directories for output files
        std::for_each(command.output_files().cbegin(),
                      command.output_files().cend(), createDirectoryIfNeeded);

        // Create parent directories for out directories
        std::for_each(command.output_directories().cbegin(),
                      command.output_directories().cend(),
                      createDirectoryIfNeeded);
    }
}

void Runner::execute(const std::vector<std::string> &command)
{
    const int exit_code = buildboxcommon::SystemUtils::executeCommand(command);
    // --------------------------------------------------------------------

    // `executeCommand()` only returns when encountering an error, so the
    // lines below will only be executed in that case:
    const char *command_name = command.front().c_str();
    perror(command_name);
    _Exit(exit_code);
}

void Runner::executeAndStore(
    const std::vector<std::string> &command,
    const UploadOutputsCallback &upload_outputs_function, ActionResult *result)
{
    std::unique_ptr<Runner_StandardOutputFile> stdoutFile, stderrFile;
    if (!d_standardOutputsCaptureConfig.skip_capture) {
        stdoutFile = std::make_unique<Runner_StandardOutputFile>(
            d_standardOutputsCaptureConfig.stdout_file_path);
        stderrFile = std::make_unique<Runner_StandardOutputFile>(
            d_standardOutputsCaptureConfig.stderr_file_path);
    }
    else {
        BUILDBOX_RUNNER_LOG(
            TRACE,
            "Will skip the capturing and uploading of stdout and stderr.");
    }

    BUILDBOX_RUNNER_LOG(
        DEBUG, "Executing command: "
                   << buildboxcommon::logging::printableCommandLine(command));

    auto *result_metadata = result->mutable_execution_metadata();

    // -- Execution started --
    result_metadata->mutable_execution_start_timestamp()->CopyFrom(
        TimeUtils::now());

    // Fork and exec
    const auto pid = fork();
    if (pid == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category, "Error in fork()");
    }
    else if (pid == 0) {
        if (stdoutFile) {
            SystemUtils::redirectStandardOutputToFile(STDOUT_FILENO,
                                                      stdoutFile->name());
        }
        if (stderrFile) {
            SystemUtils::redirectStandardOutputToFile(STDERR_FILENO,
                                                      stderrFile->name());
        }

        execute(command);
    }

    while (!getSignalStatus()) {
        const int exit_code = SystemUtils::waitPidOrSignal(pid);
        if (exit_code >= 0) {
            // -- Execution ended --
            result_metadata->mutable_execution_completed_timestamp()->CopyFrom(
                TimeUtils::now());

            if (!d_standardOutputsCaptureConfig.skip_capture) {
                // Uploading standard outputs:
                Digest stdout_digest, stderr_digest;
                std::tie(stdout_digest, stderr_digest) =
                    upload_outputs_function(stdoutFile->name(),
                                            stderrFile->name());

                result->mutable_stdout_digest()->CopyFrom(stdout_digest);
                result->mutable_stderr_digest()->CopyFrom(stderr_digest);
            }

            result->set_exit_code(exit_code);
            return;
        }
    }

    // We've received either SIGINT or SIGTERM before execution completed.
    // Immediately terminate action command.
    BUILDBOX_RUNNER_LOG(INFO, "Caught signal");
    kill(pid, SIGKILL);
    SystemUtils::waitPid(pid);
}

void Runner::executeAndStore(const std::vector<std::string> &command,
                             ActionResult *result)
{
    UploadOutputsCallback outputs_upload_function;
    if (!d_standardOutputsCaptureConfig.skip_capture) {
        // This callback will be used to upload the contents of stdout and
        // stderr.
        outputs_upload_function =
            std::bind(&Runner::uploadOutputs, this, std::placeholders::_1,
                      std::placeholders::_2);
    }

    this->executeAndStore(command, outputs_upload_function, result);
}

std::unique_ptr<StagedDirectory> Runner::stageDirectory(const Digest &digest)
{
    return this->stage(digest, d_stage_path, d_use_localcas_protocol);
}

bool Runner::parseArguments(int argc, char *argv[])
{
    // The logger instance is not yet initialized at this point, write messages
    // to std::cout/std::cerr.

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
                const std::string key(arg, static_cast<size_t>(assign - arg));
                const char *value = assign + 1;
                if (key == "action") {
                    this->d_inputPath = std::string(value);
                }
                else if (key == "action-result") {
                    this->d_outputPath = std::string(value);
                }
                else if (key == "workspace-path") {
                    this->d_stage_path = std::string(value);
                }
                else if (key == "log-level") {
                    std::string level(value);
                    std::transform(level.begin(), level.end(), level.begin(),
                                   ::tolower);
                    if (logging::stringToLogLevel.find(level) ==
                        logging::stringToLogLevel.end()) {
                        std::cerr << "Invalid log level." << std::endl;
                        return false;
                    }
                    // save the value, then set log level after logger is
                    // initialized
                    this->d_logLevel = logging::stringToLogLevel.at(level);
                }
                else if (key == "log-file") {
                    std::cerr << "Option --log-file is no longer supported. "
                                 "To redirect logs to files, use "
                                 "--log-directory=DIR.\n";
                    return false;
                }
                else if (key == "log-directory") {
                    if (!FileUtils::isDirectory(value)) {
                        std::cerr << "--log-directory: directory ["
                                  << std::string(value)
                                  << "] does not exist\n";
                        return false;
                    }

                    auto &logger = logging::Logger::getLoggerInstance();
                    logger.setOutputDirectory(value);
                }
                else if (key == "stdout-file") {
                    this->d_standardOutputsCaptureConfig.stdout_file_path =
                        std::string(value);
                }
                else if (key == "stderr-file") {
                    this->d_standardOutputsCaptureConfig.stderr_file_path =
                        std::string(value);
                }
                else {
                    std::cerr << "Invalid option " << argv[0] << std::endl;
                    return false;
                }
            }
            else {
                if (strcmp(arg, "help") == 0) {
                    usage(argv[0]);
                    printSpecialUsage();
                    exit(0);
                }
                else if (strcmp(arg, "use-localcas") == 0) {
                    std::cerr
                        << "WARNING: The --use-localcas option will be "
                           "deprecated. "
                           "LocalCAS support is now enabled by default.\n";
                    this->d_use_localcas_protocol = true;
                }
                else if (strcmp(arg, "disable-localcas") == 0) {
                    this->d_use_localcas_protocol = false;
                }
                else if (strcmp(arg, "no-logs-capture") == 0) {
                    this->d_standardOutputsCaptureConfig.skip_capture = true;
                }
                else if (strcmp(arg, "verbose") == 0) {
                    BUILDBOX_LOG_SET_LEVEL(LogLevel::DEBUG);
                }
                else if (strcmp(arg, "capabilities") == 0) {
                    // Generic capabilities
                    std::cout << "no-logs-capture\n";

                    printSpecialCapabilities();
                    exit(0);
                }
                else if (strcmp(arg, "validate-parameters") == 0) {
                    this->d_validateParametersAndExit = true;
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

std::pair<Digest, Digest>
Runner::uploadOutputs(const std::string &stdout_file,
                      const std::string &stderr_file) const
{
    Digest stdout_digest = CASHash::hashFile(stdout_file);
    Digest stderr_digest = CASHash::hashFile(stderr_file);

    const std::vector<Client::UploadRequest> upload_requests = {
        Client::UploadRequest::from_path(stdout_digest, stdout_file),
        Client::UploadRequest::from_path(stderr_digest, stderr_file),
    };

    // If some output fails to be uploaded, we'll return an empty digest for
    // it.
    std::vector<Client::UploadResult> failed_blobs;
    try {
        failed_blobs = this->d_casClient->uploadBlobs(upload_requests);
    }
    catch (const std::exception &e) {
        BUILDBOX_LOG_ERROR("Failed to upload stdout and stderr: " << e.what());
        return std::make_pair(Digest(), Digest());
    }

    for (const auto &blob : failed_blobs) {
        if (blob.digest == stdout_digest) {
            BUILDBOX_LOG_ERROR("Failed to upload stdout contents. Received: "
                               << blob.status.error_message());
            stdout_digest = Digest();
        }
        else {
            BUILDBOX_LOG_ERROR("Failed to upload stderr contents. Received: "
                               << blob.status.error_message());
            stderr_digest = Digest();
        }
    }

    return std::make_pair(std::move(stdout_digest), std::move(stderr_digest));
}
} // namespace buildboxcommon
