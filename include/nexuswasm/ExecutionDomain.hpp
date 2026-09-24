#pragma once

#ifndef NEXUSWASM_EXECUTION_DOMAIN_HEADER
#define NEXUSWASM_EXECUTION_DOMAIN_HEADER

#include <memory>
#include <string_view>
#include <utility>

#include <nexuswasm/AsyncHostFunction.hpp>
#include <nexuswasm/AsyncInstantiation.hpp>
#include <nexuswasm/Export.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct EngineState;
        struct ExecutionDomainState;
    }

    class Realm;
    class Program;

    class NEXUSWASM_API ExecutionDomain final
    {
    public:
        ExecutionDomain(const ExecutionDomain&) = delete;
        ExecutionDomain& operator=(const ExecutionDomain&) = delete;

        ExecutionDomain(ExecutionDomain&&) noexcept;
        ExecutionDomain& operator=(ExecutionDomain&&) noexcept;

        ~ExecutionDomain();

        [[nodiscard]]
        Result<Instance> Instantiate(const Module& module);

        [[nodiscard]]
        Result<AsyncInstantiation> InstantiateAsync(const Module& module);

        [[nodiscard]]
        Result<void> DefineAsyncFunction(
            std::string_view                  module,
            std::string_view                  name,
            const AsyncHostFunctionSignature& signature,
            AsyncHostFunction                 function
        );

    private:
        std::shared_ptr<detail::ExecutionDomainState> m_pState;

        explicit ExecutionDomain(std::shared_ptr<detail::ExecutionDomainState> state) noexcept;

        [[nodiscard]]
        static Result<ExecutionDomain> Create(std::shared_ptr<detail::EngineState> engine);

        friend class Realm;
        friend class Program;
    };
}

#endif
