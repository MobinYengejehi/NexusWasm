#pragma once

#ifndef NEXUSWASM_MODULE_METADATA_HEADER
#define NEXUSWASM_MODULE_METADATA_HEADER

#include <string>
#include <vector>

#include <wasmtime.h>

namespace nexus::detail
{
    struct FunctionSignature final
    {
        std::vector<wasm_valkind_t> parameters;
        std::vector<wasm_valkind_t> results;
    };

    struct ModuleImportInfo final
    {
        std::string module;
        std::string name;

        wasm_externkind_t kind{};

        FunctionSignature functionSignature;
    };

    struct ModuleExportInfo final
    {
        std::string name;

        wasm_externkind_t kind{};

        FunctionSignature functionSignature;
    };

    struct ModuleMetadata final
    {
        std::vector<ModuleImportInfo> imports;
        std::vector<ModuleExportInfo> exports;
    };

    [[nodiscard]]
    ModuleMetadata InspectModule(const wasmtime_module_t* module);

    [[nodiscard]]
    bool IsSimpleFunctionSignature(const FunctionSignature& signature) noexcept;

    [[nodiscard]]
    bool FunctionSignatureEqual(
        const FunctionSignature& lhs,
        const FunctionSignature& rhs
    ) noexcept;

    [[nodiscard]]
    std::string FormatFunctionSignature(const FunctionSignature& signature);
}

#endif
