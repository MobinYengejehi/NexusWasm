#pragma once

#ifndef NEXUSWASM_VERSION_HEADER
#define NEXUSWASM_VERSION_HEADER

#include <cstdint>

#include <nexuswasm/Export.hpp>

namespace nexus
{
    struct Version final
    {
        std::uint32_t major;
        std::uint32_t minor;
        std::uint32_t patch;
    };

    [[nodiscard]]
    NEXUSWASM_API Version GetVersion() noexcept;

    [[nodiscard]]
    NEXUSWASM_API const char* GetVersionString() noexcept;
}

#endif
