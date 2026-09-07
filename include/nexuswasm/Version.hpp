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

        constexpr Version(
            const std::uint32_t majorValue,
            const std::uint32_t minorValue,
            const std::uint32_t patchValue
        ) noexcept :
            major(majorValue),
            minor(minorValue),
            patch(patchValue)
        {}
    };

    [[nodiscard]]
    NEXUSWASM_API Version GetVersion() noexcept;

    [[nodiscard]]
    NEXUSWASM_API const char* GetVersionString() noexcept;
}

#endif
