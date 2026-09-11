#include <nexuswasm/ExecutionDomain.hpp>

#include <memory>
#include <utility>

#include "wasmtime/WasmtimeError.hpp"
#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    ExecutionDomain::ExecutionDomain(std::shared_ptr<detail::ExecutionDomainState> state) noexcept:
        m_pState{ std::move(state) }
    {}

    ExecutionDomain::~ExecutionDomain() = default;

    ExecutionDomain::ExecutionDomain(ExecutionDomain&&) noexcept = default;
    ExecutionDomain& ExecutionDomain::operator=(ExecutionDomain&&) noexcept = default;

    Result<ExecutionDomain> ExecutionDomain::Create(std::shared_ptr<detail::EngineState> engine)
    {
        if (!engine)
        {
            return Error{
                ErrorCode::InvalidState,
                "ExecutionDomain received no EngineState."
            };
        }

        detail::StorePtr store{ wasmtime_store_new(engine->engine.get(), nullptr, nullptr) };
        if (!store)
        {
            return Error{
                ErrorCode::ExecutionDomainCreationFailed,
                "Failed to create Wasmtime Store."
            };
        }

        detail::LinkerPtr linker{ wasmtime_linker_new(engine->engine.get()) };
        if (!linker)
        {
            return Error{
                ErrorCode::ExecutionDomainCreationFailed,
                "Failed to create Wasmtime Linker."
            };
        }

        wasmtime_linker_allow_shadowing(linker.get(), false);

        auto state = std::make_shared<detail::ExecutionDomainState>(std::move(engine), store.release(), linker.release());

        return ExecutionDomain{ std::move(state) };
    }

    Result<Instance> ExecutionDomain::Instantiate(const Module& module)
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "ExecutionDomain is in a moved-from state."
            };
        }

        if (!module.m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Module is in moved-from state."
            };
        }

        if (module.m_pState->engine.get() != m_pState->engine.get())
        {
            return Error{
                ErrorCode::RuntimeMismatch,
                "Module blongs to another runtime."
            };
        }

        wasmtime_context_t* context = wasmtime_store_context(m_pState->store.get());

        wasmtime_instance_t rawInstance{};
        wasm_trap_t*        trap = nullptr;
        wasmtime_error_t*   error = wasmtime_linker_instantiate(
            m_pState->linker.get(),
            context,
            module.m_pState->module.get(),
            &rawInstance,
            &trap
        );

        if (error != nullptr)
        {
            return Error{
                ErrorCode::InstantiationFailed,
                detail::TakeWasmtimeError(error)
            };
        }
        if (trap != nullptr)
        {
            return Error{
                ErrorCode::Trap,
                detail::TakeWasmtimeTrap(trap)
            };
        }

        auto state = std::make_unique<detail::InstanceState>(m_pState, rawInstance);

        return Instance{ std::move(state) };
    }
}
