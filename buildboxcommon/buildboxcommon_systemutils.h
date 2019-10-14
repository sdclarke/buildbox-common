#ifndef INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS
#define INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS

#include <string>
#include <vector>

namespace buildboxcommon {

struct SystemUtils {
    /*
     * Executes the given command in the current process. `command[0]` must be
     * a command name and the other entries its arguments.
     *
     * If successful, it does not return.
     *
     * On error it returns exit codes that follow the convention used by Bash:
     * 126 if the command could not be executed (for example for lack of
     * permissions), or 127 if the command could not be found.
     */
    static int executeCommand(const std::vector<std::string> &command);

    /*
     * Waits for the given PID and returns an exit code following the
     * convention used by Bash:
     *  - If it exits: its status code,
     *  - If it is signaled: 128 + the signal number
     *
     * On errors, throws an `std::system_error` exception.
     */
    static int waitPid(const pid_t pid);
};

} // namespace buildboxcommon

#endif // INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS
