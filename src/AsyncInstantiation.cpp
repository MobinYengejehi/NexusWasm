#include <nexuswasm/AsyncInstantiation.hpp>
#include <nexuswasm/ExecutionDomain.hpp>

#include <memory>
#include <utility>

#include <wasmtime.h>

#include "execution/StoreExecutionState.hpp"

#include "wasmtime/WasmtimeError.hpp"
#include "wasmtime/WasmtimeState.hpp"

namespace nexus::detail
{
    struct AsyncInstantiationState final
    {
        std::shared_ptr<ExecutionDomainState> domain;
        std::shared_ptr<ModuleState>          module;

        StoreExecutionLease executionLease;
        wasmtime_instance_t instance{};
        wasm_trap_t*        trap = nullptr;
        wasmtime_error_t*   error = nullptr;

        CallFuturePtr future;

        bool ready = false;
        bool instanceToken = false;

        AsyncInstantiationState(
            std::shared_ptr<ExecutionDomainState> domainState,
            std::shared_ptr<ModuleState>          moduleState,
            StoreExecutionLease                   lease
        ):
            domain{ std::move(domainState) },
            module{ std::move(moduleState) },
            executionLease{ std::move(lease) }
        {}

        ~AsyncInstantiationState()
        {
            future.reset();
            executionLease.Release();

            if (error != nullptr)
            {
                wasmtime_error_delete(error);
                error = nullptr;
            }

            if (trap != nullptr)
            {
                wasm_trap_delete(trap);
                trap = nullptr;
            }
        }
    };
}

namespace nexus
{
    Result<AsyncInstantiation> ExecutionDomain::InstantiateAsync(const Module& module)
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "ExecutionDomain is in "
                "a moved-from state."
            };
        }

        if (!module.m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Module is in a moved-from state."
            };
        }

        if (module.m_pState->engine.get() != m_pState->engine.get())
        {
            return Error{
                ErrorCode::RuntimeMismatch,
                "Module belongs to another Runtime."
            };
        }

        auto leaseResult = m_pState->executionState.Acquire(
            detail::StoreExecutionOperation::AsynchronousExecution,
            "asynchronous module instantiation"
        );
        if (!leaseResult)
        {
            return leaseResult.GetError();
        }

        auto executionLease = std::move(leaseResult).Value();

        if (m_pState->UsesAsyncHostFunction(module.m_pState->metadata))
        {
            m_pState->executionState.MarkAsyncRequired();
        }

        auto state = std::make_shared<detail::AsyncInstantiationState>(
            m_pState,
            module.m_pState,
            std::move(executionLease)
        );

        wasmtime_context_t* context = wasmtime_store_context(m_pState->store.get());

        state->future.reset(
            wasmtime_linker_instantiate_async(
                m_pState->linker.get(),
                context,
                module.m_pState->module.get(),
                &state->instance,
                &state->trap,
                &state->error
            )
        );

        if (!state->future)
        {
            return Error{
                ErrorCode::InstantiationFailed,
                "Wasmtime did not return an "
                "asynchronous instantiation future."
            };
        }

        return AsyncInstantiation{ std::move(state) };
    }

    Result<AsyncPollStatus> AsyncInstantiation::Poll()
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "AsyncInstantiation is in "
                "a moved-from state."
            };
        }

        if (m_pState->ready)
        {
            return AsyncPollStatus::Ready;
        }

        if (!m_pState->future)
        {
            return Error{
                ErrorCode::InvalidState,
                "AsyncInstantiation contains "
                "no Wasmtime future."
            };
        }

        const bool completed = wasmtime_call_future_poll(m_pState->future.get());
        if (!completed)
        {
            return AsyncPollStatus::Pending;
        }

        m_pState->future.reset();
        m_pState->executionLease.Release();

        m_pState->ready = true;

        return AsyncPollStatus::Ready;
    }

    Result<Instance> AsyncInstantiation::TakeInstance()
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "AsyncInstantiation is in "
                "a moved-from state."
            };
        }

        if (!m_pState->ready)
        {
            return Error{
                ErrorCode::AsyncOperationNotReady,
                "Asynchronous instantiation "
                "is still pending."
            };
        }

        if (m_pState->instanceToken)
        {
            return Error{
                ErrorCode::AsyncResultAlreadyTaken,
                "Asynchronous instantiation "
                "result has already been taken."
            };
        }

        m_pState->instanceToken = true;

        if (m_pState->error != nullptr)
        {
            wasmtime_error_t* error = std::exchange(m_pState->error, nullptr);
            return Error{
                ErrorCode::InstantiationFailed,
                detail::TakeWasmtimeError(error)
            };
        }

        if (m_pState->trap != nullptr)
        {
            wasm_trap_t* trap = std::exchange(m_pState->trap, nullptr);
            return Error{
                ErrorCode::Trap,
                detail::TakeWasmtimeTrap(trap)
            };
        }

        auto state = std::make_unique<detail::InstanceState>(m_pState->domain, m_pState->instance);

        return Instance{ std::move(state) };
    }
}
