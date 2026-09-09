#pragma once

#ifndef NEXUSWASM_COMPILED_MODULE_HEADER
#define NEXUSWASM_COMPILED_MODULE_HEADER

#include <memory>

#include <nexuswasm/Export.hpp>

namespace nexus
{
    namespace detail
    {
        struct ModuleState;
    }

    class Runtime;

    class NEXUSWASM_API CompiledModule final
    {
    public:
        CompiledModule(const CompiledModule&) noexcept;
        CompiledModule& operator=(const CompiledModule&) noexcept;

        CompiledModule(CompiledModule&&) noexcept;
        CompiledModule& operator=(CompiledModule&&) noexcept;

        ~CompiledModule();

    private:
        explicit CompiledModule(std::shared_ptr<detail::ModuleState> state) noexcept;

        std::shared_ptr<detail::ModuleState> m_pState;

        friend class Runtime;
    };
}

#endif
