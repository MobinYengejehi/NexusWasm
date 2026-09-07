#include <nexuswasm/Version.hpp>

namespace nexus
{
    Version GetVersion() noexcept
    {
        return Version(
            NEXUSWASM_VERSION_MAJOR,
            NEXUSWASM_VERSION_MINOR,
            NEXUSWASM_VERSION_PATCH
        );
    }

    const char* GetVersionString() noexcept
    {
        return NEXUSWASM_VERSION_STRING;
    }
}
