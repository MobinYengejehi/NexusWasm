#include <nexuswasm/Instance.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <wasmtime.h>

#include "execution/StoreExecutionState.hpp"

#include "wasmtime/WasmtimeState.hpp"
#include "wasmtime/WasmtimeError.hpp"

namespace nexus
{
    namespace
    {
        wasm_valkind_t ToWasmKind(const detail::ScalerKind kind)
        {
            switch (kind)
            {
            case detail::ScalerKind::I32:
                return WASM_I32;
            case detail::ScalerKind::I64:
                return WASM_I64;
            case detail::ScalerKind::F32:
                return WASM_F32;
            case detail::ScalerKind::F64:
                return WASM_F64;
            }

            return WASM_I32;
        }

        wasmtime_val_t ToWasmtimeValue(const detail::ScalerValue& value)
        {
            wasmtime_val_t result{};

            switch (value.kind)
            {
            case detail::ScalerKind::I32:
                result.kind = WASMTIME_I32;
                result.of.i32 = value.i32;
                break;
            case detail::ScalerKind::I64:
                result.kind = WASMTIME_I64;
                result.of.i64 = value.i64;
                break;
            case detail::ScalerKind::F32:
                result.kind = WASMTIME_F32;
                result.of.f32 = value.f32;
                break;
            case detail::ScalerKind::F64:
                result.kind = WASMTIME_F64;
                result.of.f64 = value.f64;
                break;
            }

            return result;
        }

        detail::ScalerValue FromWasmtimeValue(const wasmtime_val_t& value)
        {
            detail::ScalerValue result{};

            switch (value.kind)
            {
            case WASMTIME_I32:
                result.kind = detail::ScalerKind::I32;
                result.i32 = value.of.i32;
                break;
            case WASMTIME_I64:
                result.kind = detail::ScalerKind::I64;
                result.i64 = value.of.i64;
                break;
            case WASMTIME_F32:
                result.kind = detail::ScalerKind::F32;
                result.f32 = value.of.f32;
                break;
            case WASMTIME_F64:
                result.kind = detail::ScalerKind::F64;
                result.f64 = value.of.f64;
                break;
            default:
                result.kind = detail::ScalerKind::I32;
                result.i32 = 0;
                break;
            }

            return result;
        }

        class ResolvedFunction final
        {
        public:
            explicit ResolvedFunction(const wasmtime_extern_t& item) noexcept:
                m_cItem{ item },
                m_bValid{ true }
            {}

            ResolvedFunction(const ResolvedFunction&) = delete;
            ResolvedFunction& operator=(const ResolvedFunction&) = delete;

            ResolvedFunction(ResolvedFunction&& other) noexcept:
                m_cItem{ other.m_cItem },
                m_bValid{ std::exchange(other.m_bValid, false) }
            {}

            ResolvedFunction& operator=(ResolvedFunction&& other) noexcept
            {
                if (this == &other)
                {
                    return *this;
                }

                Reset();

                m_cItem = other.m_cItem;
                m_bValid = std::exchange(other.m_bValid, false);

                return *this;
            }

            ~ResolvedFunction()
            {
                Reset();
            }

            [[nodiscard]]
            const wasmtime_func_t* Function() const noexcept
            {
                return &m_cItem.of.func;
            }

        private:
            wasmtime_extern_t m_cItem{};
            bool              m_bValid = false;

            void Reset() noexcept
            {
                if (!m_bValid)
                {
                    return;
                }

                wasmtime_extern_delete(&m_cItem);

                m_bValid = false;
            }
        };

        Result<ResolvedFunction> ResolveFunction(
            wasmtime_context_t*        context,
            const wasmtime_instance_t& instance,
            const std::string_view     name,
            const detail::ScalerValue* args,
            const std::size_t          argumentCount,
            const detail::ScalerKind   expectedResult
        )
        {
            wasmtime_extern_t exportItem{};

            const bool found = wasmtime_instance_export_get(
                context,
                &instance,
                name.data(),
                name.size(),
                &exportItem
            );

            if (!found)
            {
                return Error{
                    ErrorCode::ExportNotFound,
                    "WebAssembly export was not found."
                };
            }

            ResolvedFunction resolved{ exportItem };

            if (exportItem.kind != WASMTIME_EXTERN_FUNC)
            {
                return Error{
                    ErrorCode::ExportNotFunction,
                    "WebAssembly export is not a function."
                };
            }

            wasm_functype_t* functionType = wasmtime_func_type(context, resolved.Function());
            if (functionType == nullptr)
            {
                return Error{
                    ErrorCode::CallFailed,
                    "Failed to inspect WebAssembly function type."
                };
            }

            struct FunctionTypeGuard final
            {
                wasm_functype_t* value;

                ~FunctionTypeGuard()
                {
                    wasm_functype_delete(value);
                }
            };

            FunctionTypeGuard typeGuard{ functionType };

            const wasm_valtype_vec_t* parameters = wasm_functype_params(functionType);
            const wasm_valtype_vec_t* results = wasm_functype_results(functionType);

            if (parameters->size != argumentCount)
            {
                return Error{
                    ErrorCode::SignatureMismatch,
                    "WebAssembly function parameter count mismatch."
                };
            }

            if (results->size != 1)
            {
                return Error{
                    ErrorCode::SignatureMismatch,
                    "NexusWasm scalar call currently "
                    "requires exactly one result."
                };
            }

            for (std::size_t i = 0; i < argumentCount; ++i)
            {
                const wasm_valkind_t actual = wasm_valtype_kind(parameters->data[i]);
                const wasm_valkind_t expected = ToWasmKind(args[i].kind);

                if (actual != expected)
                {
                    return Error{
                        ErrorCode::SignatureMismatch,
                        "WebAssembly function parameter "
                        "type mismatch."
                    };
                }
            }

            if (wasm_valtype_kind(results->data[0]) != ToWasmKind(expectedResult))
            {
                return Error{
                    ErrorCode::SignatureMismatch,
                    "WebAssembly function result "
                    "type mismatch."
                };
            }

            return resolved;
        }

        std::vector<wasmtime_val_t> PackWasmtimeArguments(
            const detail::ScalerValue* args,
            const std::size_t          argumentCount
        )
        {
            std::vector<wasmtime_val_t> results;
            results.reserve(argumentCount);

            for (std::size_t i = 0; i < argumentCount; ++i)
            {
                results.push_back(ToWasmtimeValue(args[i]));
            }

            return results;
        }
    }

    namespace detail
    {
        struct AsyncCallState final
        {
            std::shared_ptr<ExecutionDomainState> domain;

            StoreExecutionLease executionLease;
            ResolvedFunction    function;

            std::vector<wasmtime_val_t> arguments;
            wasmtime_val_t              result{};

            wasm_trap_t*      trap = nullptr;
            wasmtime_error_t* error = nullptr;

            CallFuturePtr future;

            bool ready = false;
            bool resultTaken = false;

            AsyncCallState(
                std::shared_ptr<ExecutionDomainState> domainState,
                StoreExecutionLease                   lease,
                ResolvedFunction                      resolvedFunction,
                std::vector<wasmtime_val_t>           packedArguments
            ):
                domain{ std::move(domainState) },
                executionLease{ std::move(lease) },
                function{ std::move(resolvedFunction) },
                arguments{ std::move(packedArguments) }
            {}

            ~AsyncCallState()
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

        Result<AsyncPollStatus> PollAsyncCall(const std::shared_ptr<AsyncCallState>& state)
        {
            if (!state)
            {
                return Error{
                    ErrorCode::InvalidState,
                    "AsyncCall is in a moved-from state."
                };
            }

            if (state->ready)
            {
                return AsyncPollStatus::Ready;
            }

            if (!state->future)
            {
                return Error{
                    ErrorCode::InvalidState,
                    "AsyncCall contains no Wasmtime future."
                };
            }

            const bool complete = wasmtime_call_future_poll(state->future.get());
            if (!complete)
            {
                return AsyncPollStatus::Pending;
            }

            state->future.reset();
            state->executionLease.Release();

            state->ready = true;

            return AsyncPollStatus::Ready;
        }

        Result<ScalerValue> TakeAsyncCallResult(const std::shared_ptr<AsyncCallState>& state)
        {
            if (!state)
            {
                return Error{
                    ErrorCode::InvalidState,
                    "AsyncCall is in a moved-from state."
                };
            }

            if (!state->ready)
            {
                return Error{
                    ErrorCode::AsyncResultAlreadyTaken,
                    "Async call result has already been taken."
                };
            }

            state->resultTaken = true;

            if (state->error != nullptr)
            {
                wasmtime_error_t* error = std::exchange(state->error, nullptr);
                return Error{
                    ErrorCode::CallFailed,
                    TakeWasmtimeError(error)
                };
            }

            if (state->trap != nullptr)
            {
                wasm_trap_t* trap = std::exchange(state->trap, nullptr);
                return Error{
                    ErrorCode::Trap,
                    TakeWasmtimeTrap(trap)
                };
            }

            return FromWasmtimeValue(state->result);
        }
    }

    Instance::Instance(std::unique_ptr<detail::InstanceState> state) noexcept:
        m_pState{ std::move(state) }
    {}

    Instance::~Instance() = default;

    Instance::Instance(Instance&&) noexcept = default;
    Instance& Instance::operator=(Instance&&) noexcept = default;

    Result<detail::ScalerValue> Instance::CallScaler(
        const std::string_view     name,
        const detail::ScalerValue* args,
        const std::size_t          argumentCount,
        const detail::ScalerKind   expectedResult
    )
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Instance is in a moved-from state."
            };
        }

        auto domain = m_pState->domain.lock();
        if (!domain)
        {
            return Error{
                ErrorCode::InvalidState,
                "The owning Program or ExecutionDomain "
                "has been destroyed."
            };
        }

        auto leaseResult = domain->executionState.Acquire(
            detail::StoreExecutionOperation::SynchronousExecution,
            "synchronous function call"
        );

        if (!leaseResult)
        {
            return leaseResult.GetError();
        }

        auto executionLease = std::move(leaseResult).Value();

        wasmtime_context_t* context = wasmtime_store_context(domain->store.get());

        auto functionResult = ResolveFunction(
            context,
            m_pState->instance,
            name,
            args,
            argumentCount,
            expectedResult
        );

        if (!functionResult)
        {
            return functionResult.GetError();
        }

        auto              function = std::move(functionResult).Value();
        auto              wasmtimeArguments = PackWasmtimeArguments(args, argumentCount);
        wasmtime_val_t    result{};
        wasm_trap_t*      trap = nullptr;
        wasmtime_error_t* error = wasmtime_func_call(
            context,
            function.Function(),
            wasmtimeArguments.data(),
            wasmtimeArguments.size(),
            &result,
            1,
            &trap
        );

        if (error != nullptr)
        {
            return Error{
                ErrorCode::CallFailed,
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

        return FromWasmtimeValue(result);
    }

    Result<std::shared_ptr<detail::AsyncCallState>> Instance::CallScalerAsync(
        const std::string_view     name,
        const detail::ScalerValue* args,
        const std::size_t          argumentCount,
        const detail::ScalerKind   expectedResult
    )
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Instance is in a moved-from state."
            };
        }

        auto domain = m_pState->domain.lock();
        if (!domain)
        {
            return Error{
                ErrorCode::InvalidState,
                "The owning Program or ExecutionDomain "
                "has been destroyed."
            };
        }

        auto leaseResult = domain->executionState.Acquire(
            detail::StoreExecutionOperation::AsynchronousExecution,
            "asynchronous function call"
        );
        if (!leaseResult)
        {
            return leaseResult.GetError();
        }

        auto executionLease = std::move(leaseResult).Value();

        wasmtime_context_t* context = wasmtime_store_context(domain->store.get());

        auto functionResult = ResolveFunction(
            context,
            m_pState->instance,
            name,
            args,
            argumentCount,
            expectedResult
        );
        if (!functionResult)
        {
            return functionResult.GetError();
        }

        auto function = std::move(functionResult).Value();
        auto wasmtimeArguments = PackWasmtimeArguments(args, argumentCount);
        auto state = std::make_shared<detail::AsyncCallState>(
            domain,
            std::move(executionLease),
            std::move(function),
            std::move(wasmtimeArguments)
        );

        state->future.reset(
            wasmtime_func_call_async(
                context,
                state->function.Function(),
                state->arguments.data(),
                state->arguments.size(),
                &state->result,
                1,
                &state->trap,
                &state->error
            )
        );

        if (!state->future)
        {
            return Error{
                ErrorCode::CallFailed,
                "Wasmtime did not return an "
                "asynchronous call future."
            };
        }

        return state;
    }
}
