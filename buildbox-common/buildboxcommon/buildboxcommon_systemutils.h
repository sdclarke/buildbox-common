#ifndef INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS
#define INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS

#include <string>
#include <vector>

namespace buildboxcommon {

struct SystemUtils {
    /*
     * Executes the given command in the current process. `command[0]` must be
     * a path to a binary and the other entries its arguments.
     *
     * If successful, it does not return.
     *
     * On error it returns exit codes that follow the convention used by Bash:
     * 126 if the command could not be executed (for example for lack of
     * permissions), or 127 if the command could not be found.
     */
    static int executeCommand(const std::vector<std::string> &command);

    /* Looks for the absolute path to a given command using the `$PATH`
     * environment variable.
     *
     * If `command` is already a path, it returns it unmodified.
     *
     * If the corresponding executable is not found, returns an empty string.
     */
    static std::string getPathToCommand(const std::string &command);

    /*
     * Waits for the given PID and returns an exit code following the
     * convention used by Bash:
     *  - If it exits: its status code,
     *  - If it is signaled: 128 + the signal number
     *
     * On errors, throws an `std::system_error` exception.
     */
    static int waitPid(const pid_t pid);

    /*
     * Equivalent to `waitPid()` except that it returns `-EINTR` if a signal
     * was caught.
     */
    static int waitPidOrSignal(const pid_t pid);

    /*
     * Get current working directory. Uses getcwd()
     *
     * On error, throws as 'std::runtime_error` exception
     */
    static std::string get_current_working_directory();

    /*
     * Redirects stdout or stderr to a given path. The file
     * will be created if necessary and truncated if already exists (equivalent
     * to a > redirection).
     *
     * If the specified file descriptor is not STDOUT_FILENO or STDERR_FILENO,
     * throws `std::invalid_argument`.
     *
     * On errors throws `std::system_error`.
     */
    static void redirectStandardOutputToFile(const int standardOutputFd,
                                             const std::string &path);
};

} // namespace buildboxcommon

#endif // INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS
