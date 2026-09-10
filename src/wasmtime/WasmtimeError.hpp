#pragma once

#ifndef NEXUSWASM_WASMTIME_ERROR_HEADER
#define NEXUSWASM_WASMTIME_ERROR_HEADER

#include <string>

#include <wasmtime.h>

namespace nexus::detail
{
    [[nodiscard]]
    std::string TakeWasmtimeError(wasmtime_error_t* error);

    [[nodiscard]]
    std::string TakeWasmtimeTrap(wasm_trap_t* trap);
}

#endif
