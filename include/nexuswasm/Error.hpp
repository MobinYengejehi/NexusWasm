#pragma once

#ifndef NEXUSWASM_ERROR_HEADER
#define NEXUSWASM_ERROR_HEADER

#include <string>
#include <utility>

namespace nexus
{
    enum class ErrorCode
    {
        None = 0,

        EngineCreationFailed,
        ExecutionDomainCreationFailed,

        CompilationFailed,
        InstantiationFailed,

        RuntimeMismatch,
        InvalidState,

        ExportNotFound,
        ExportNotFunction,

        SignatureMismatch,

        Trap,
        CallFailed,

        InvalidModuleNamespace,
        DuplicateModuleNamespace,

        ModuleGraphEmpty,
        ModuleGraphAlreadyInstantiated,
        ModuleGraphNotInstantiated,
        ModuleGraphFailed,

        UnresolvedImport,

        UnsupportedDirectImportKind,

        MissingDependencyExport,
        ImportKindMismatch,
        ImportSignatureMismatch,

        CyclicModuleDependency,

        LinkerDefinitionFailed,

        ModuleInstanceNotFound,

        StoreBusy,
        StoreRequiresAsync,

        AsyncOperationNotReady,
        AsyncResultAlreadyTaken,

        AsyncHostFunctionDefinitionFailed
    };

    class Error final
    {
    public:
        Error(const ErrorCode code, std::string message):
            m_eCode{ code },
            m_sMessage{ std::move(message) }
        {}

        [[nodiscard]]
        ErrorCode Code() const noexcept
        {
            return m_eCode;
        }

        [[nodiscard]]
        const std::string& Message() const noexcept
        {
            return m_sMessage;
        }

    private:
        ErrorCode   m_eCode;
        std::string m_sMessage;
    };
}

#endif
