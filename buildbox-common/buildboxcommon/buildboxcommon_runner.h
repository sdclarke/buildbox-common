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

#ifndef INCLUDED_BUILDBOXCOMMON_RUNNER
#define INCLUDED_BUILDBOXCOMMON_RUNNER

#include <atomic>
#include <signal.h>

#include <buildboxcommon_client.h>
#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_stageddirectory.h>
#include <buildboxcommon_timeutils.h>

namespace buildboxcommon {

// To avoid repetition, we define these macros that will augment the messages
// that are logged with the action digest that a `Runner` instance is
// processing.
#define BUILDBOX_RUNNER_LOG_MESSAGE_FORMAT(message)                           \
    "[actionDigest=" << this->d_action_digest << "] " << message

#define BUILDBOX_RUNNER_LOG(level, message)                                   \
    {                                                                         \
        BUILDBOX_LOG_##level(BUILDBOX_RUNNER_LOG_MESSAGE_FORMAT(message));    \
    }

class Runner {
  public:
    /**
     * Execute the given Command in the given input root and return an
     * ActionResult. Subclasses should override this to implement
     * sandboxing behaviors.
     */
    virtual ActionResult execute(const Command &command,
                                 const Digest &inputRootDigest) = 0;

    /**
     * Subclasses can override this to add support for special arguments.
     * Return true if an argument was handled successfully.
     */
    virtual bool parseArg(const char *) { return false; }

    /**
     * Subclasses can override this to print a message after Runner prints
     * its usage message.
     */
    virtual void printSpecialUsage() {}

    /**
     * Subclasses can override this to print Runner-specific capabilities.
     * The format is one capability name per line. In the common case where the
     * capability is associated with a CLI option, the printed capability name
     * should match the name of the option.
     */
    virtual void printSpecialCapabilities() {}

    static void handleSignal(int signal);
    static sig_atomic_t getSignalStatus();
    /**
     * Chmod a directory and all subdirectories recursively.
     */
    static void recursively_chmod_directories(const char *path, mode_t mode);

    int main(int argc, char *argv[]);
    virtual ~Runner(){};

  protected:
    /**
     * Execute the given command (without attempting to sandbox it) and
     * store its stdout, stderr, and exit code in the given ActionResult.
     */
    void executeAndStore(const std::vector<std::string> &command,
                         ActionResult *result);
    /**
     * Stage the directory with the given digest, to stage_path, and
     * return a StagedDirectory object representing it.
     * If stage_path is empty, will stage directory to tmpdir.
     *
     * If `use_localcas_protocol` is `true` uses `LocalCasStagedDirectory`
     * instead of `FallBackStagedDirectory`.
     */
    std::unique_ptr<StagedDirectory> stageDirectory(const Digest &digest);

    // These two versions should be deprecated, and the version above used
    // instead. (Arguments are already member variables.)
    std::unique_ptr<StagedDirectory> stage(const Digest &directoryDigest,
                                           const std::string &stage_path = "",
                                           bool use_localcas_protocol = false);

    std::unique_ptr<StagedDirectory> stage(const Digest &directoryDigest,
                                           bool use_localcas_protocol = false);

    /**
     * Create parent output directories, in staged directory, as specified
     * by command
     *
     * Given an output file or directory, creates all the parent
     * directories leading up to the directory or file. But not including
     * it. The output files and directories should be relative to
     * workingDir. They should also not contain any trailing or leading
     * slashes.
     */
    void createOutputDirectories(const Command &command,
                                 const std::string &workingDir) const;

    std::shared_ptr<Client> d_casClient =
        std::shared_ptr<Client>(new Client());

    bool d_verbose;
    bool d_use_localcas_protocol = false;
    std::string d_stage_path = "";

    Digest d_action_digest;

    // Helpers to set the timestamps of the different stages that are carried
    // out in the runner. Those will be returned in the `ActionResult`, as part
    // of its `ExecutedActionMetadata` field.
    inline static void
    metadata_mark_input_download_start(ExecutedActionMetadata *metadata)
    {
        set_timestamp_to_now(metadata->mutable_input_fetch_start_timestamp());
    }

    inline static void
    metadata_mark_input_download_end(ExecutedActionMetadata *metadata)
    {
        set_timestamp_to_now(
            metadata->mutable_input_fetch_completed_timestamp());
    }

    inline static void
    metadata_mark_output_upload_start(ExecutedActionMetadata *metadata)
    {
        set_timestamp_to_now(
            metadata->mutable_output_upload_start_timestamp());
    }

    inline static void
    metadata_mark_output_upload_end(ExecutedActionMetadata *metadata)
    {
        set_timestamp_to_now(
            metadata->mutable_output_upload_completed_timestamp());
    }

  private:
    /**
     * Attempt to parse all of the given arguments and update this object
     * to reflect them. If an argument is invalid or missing, return false.
     * Otherwise, return true.
     */
    bool parseArguments(int argc, char *argv[]);

    /**
     * If the given string is larger than the maximum size, upload it to
     * CAS, store its digest in the given digest pointer, and clear it.
     */
    void uploadIfNeeded(std::string *str, Digest *digest) const;

    ConnectionOptions d_casRemote;
    std::string d_inputPath;
    std::string d_outputPath;

    static volatile sig_atomic_t d_signal_status;

    std::array<int, 2> createPipe() const;

    /**
     * These helpers `exit(1)` on failures.
     */
    void registerSignals() const;
    Action readAction(const std::string &path) const;
    void initializeCasClient() const;

    void writeActionResult(const ActionResult &action_result,
                           const std::string &path) const;

    // Given file descriptors to `stdout` and `stderr` pipes' reading ends,
    // get their contents and add them to the `ActionResult`.
    static void writeStandardStreamsToResult(const int stdout_read_fd,
                                             const int stderr_read_fd,
                                             ActionResult *result);

    // Fetch a `Command` message from the remote CAS. If that fails, log
    // the error and `exit(1)`.
    Command fetchCommand(const Digest &command_digest) const;

    // Populates a `protobuf::Timestamp` with the current time.
    inline static void set_timestamp_to_now(google::protobuf::Timestamp *t)
    {
        t->CopyFrom(buildboxcommon::TimeUtils::now());
    }

}; // namespace buildboxcommon

#define BUILDBOX_RUNNER_MAIN(x)                                               \
    int main(int argc, char *argv[]) { x().main(argc, argv); }

} // namespace buildboxcommon

#endif
