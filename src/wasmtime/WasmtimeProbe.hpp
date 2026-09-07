#pragma once

#ifndef NEXUSWASM_WASMTIME_PROPE_HEADER
#define NEXUSWASM_WASMTIME_PROPE_HEADER

#include <cstdint>
#include <string>
#include <string_view>

namespace nexus::detail
{
    enum class WasmtimeProbeFailure
    {
        None,

        EngineCreation,
        WatParsing,
        ModuleCompilation,
        StoreCreation,
        Instantiation,

        ExportNotFound,
        ExportNotFunction,

        CallError,
        Trap,
        InvalidResult
    };

    struct WasmtimeProbeResult final
    {
        bool success = false;

        std::int32_t value = 0;

        WasmtimeProbeFailure failure = WasmtimeProbeFailure::None;

        std::string message;
    };

    [[nodiscard]]
    WasmtimeProbeResult RunWasmtimeI32Binary(
        std::string_view wat,
        std::string_view exportName,
        std::int32_t     lhs,
        std::int32_t     rhs
    );
}

#endif
