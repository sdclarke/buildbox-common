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
};

} // namespace buildboxcommon

#endif // INCLUDED_BUILDBOXCOMMON_SYSTEMUTILS
