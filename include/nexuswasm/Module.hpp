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
        class  ModuleGraph;
    }

    class Runtime;
    class ExecutionDomain;

    class NEXUSWASM_API Module final
    {
    public:
        Module(const Module&) noexcept;
        Module& operator=(const Module&) noexcept;

        Module(Module&&) noexcept;
        Module& operator=(Module&&) noexcept;

        ~Module();

    private:
        explicit Module(std::shared_ptr<detail::ModuleState> state) noexcept;

        std::shared_ptr<detail::ModuleState> m_pState;

        friend class Runtime;
        friend class ExecutionDomain;
        friend class detail::ModuleGraph;
    };
}

#endif
