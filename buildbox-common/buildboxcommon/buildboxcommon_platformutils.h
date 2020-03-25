#ifndef INCLUDED_BUILDBOXCOMMON_PLATFORMUTILS
#define INCLUDED_BUILDBOXCOMMON_PLATFORMUTILS

#include <string>
#include <vector>

namespace buildboxcommon {

struct PlatformUtils {
    /**
     * Return REAPI OSFamily of running system.
     */
    static std::string getHostOSFamily();

    /**
     * Return REAPI ISA of running system.
     */
    static std::string getHostISA();
};

} // namespace buildboxcommon

#endif // INCLUDED_BUILDBOXCOMMON_PLATFORMUTILS
