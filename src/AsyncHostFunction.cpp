#include <nexuswasm/ExecutionDomain.hpp>

#include <exception>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <wasmtime.h>

#include "execution/StoreExecutionState.hpp"

#include "wasmtime/WasmtimeError.hpp"
#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    namespace
    {
        wasm_valtype_t* CreateValueType(const HostValueKind kind)
        {
            switch (kind)
            {
            case HostValueKind::I32:
                return wasm_valtype_new_i32();
            case HostValueKind::I64:
                return wasm_valtype_new_i64();
            case HostValueKind::F32:
                return wasm_valtype_new_f32();
            case HostValueKind::F64:
                return wasm_valtype_new_f64();
            }

            return nullptr;
        }

        bool ReadHostValue(
            const HostValueKind   expected,
            const wasmtime_val_t& value,
            HostValue&            output
        )
        {
            switch (expected)
            {
            case HostValueKind::I32:
            {
                if (value.kind != WASMTIME_I32)
                {
                    return false;
                }

                output = value.of.i32;

                return true;
            }
            case HostValueKind::I64:
            {
                if (value.kind != WASMTIME_I64)
                {
                    return false;
                }

                output = value.of.i64;

                return true;
            }
            case HostValueKind::F32:
            {
                if (value.kind != WASMTIME_F32)
                {
                    return false;
                }

                output = value.of.f32;

                return true;
            }
            case HostValueKind::F64:
            {
                if (value.kind != WASMTIME_F64)
                {
                    return false;
                }

                output = value.of.f64;

                return true;
            }
            }

            return false;
        }

        bool WriteHostValue(
            const HostValueKind expected,
            const HostValue&    value,
            wasmtime_val_t&     output
        )
        {
            switch (expected)
            {
            case HostValueKind::I32:
            {
                const auto* typed = std::get_if<std::int32_t>(&value);
                if (typed == nullptr)
                {
                    return false;
                }

                output.kind = WASMTIME_I32;
                output.of.i32 = *typed;

                return true;
            }
            case HostValueKind::I64:
            {
                const auto* typed = std::get_if<std::int64_t>(&value);
                if (typed == nullptr)
                {
                    return false;
                }

                output.kind = WASMTIME_I64;
                output.of.i64 = *typed;

                return true;
            }
            case HostValueKind::F32:
            {
                const auto* typed = std::get_if<float>(&value);
                if (typed == nullptr)
                {
                    return false;
                }

                output.kind = WASMTIME_F32;
                output.of.f32 = *typed;

                return true;
            }
            case HostValueKind::F64:
            {
                const auto* typed = std::get_if<double>(&value);
                if (typed == nullptr)
                {
                    return false;
                }

                output.kind = WASMTIME_F64;
                output.of.f64 = *typed;

                return true;
            }
            }

            return false;
        }

        wasm_functype_t* CreateFunctionType(const AsyncHostFunctionSignature& signature)
        {
            wasm_valtype_vec_t parameters{};
            wasm_valtype_vec_new_uninitialized(&parameters, signature.parameters.size());

            for (std::size_t i = 0; i < signature.parameters.size(); ++i)
            {
                parameters.data[i] = CreateValueType(signature.parameters[i]);
            }

            wasm_valtype_vec_t results{};
            wasm_valtype_vec_new_uninitialized(&results, signature.results.size());

            for (std::size_t i = 0; i < signature.results.size(); ++i)
            {
                results.data[i] = CreateValueType(signature.results[i]);
            }

            return wasm_functype_new(&parameters, &results);
        }

        void SetTrap(
            wasm_trap_t**      trapRet,
            const std::string& message
        ) noexcept
        {
            if (trapRet == nullptr)
            {
                return;
            }
            if (*trapRet != nullptr)
            {
                return;
            }

            *trapRet = wasmtime_trap_new(message.data(), message.size());
        }

        bool ImmediateContinuation(void*)
        {
            return true;
        }

        void ConfigureImmediateContinuation(wasmtime_async_continuation_t* continuation)
        {
            continuation->callback = &ImmediateContinuation;
            continuation->env = nullptr;
            continuation->finalizer = nullptr;
        }

        struct AsyncHostBinding final
        {
            AsyncHostFunctionSignature signature;
            AsyncHostFunction          function;
        };

        struct ContinuationState final
        {
            AsyncHostOperation operation;

            std::vector<HostValueKind> resultKinds;
            wasmtime_val_t*            results;
            std::size_t                resultCount;

            wasm_trap_t** trapRet;

            ContinuationState(
                AsyncHostOperation         asyncOperation,
                std::vector<HostValueKind> kinds,
                wasmtime_val_t*            resultStorage,
                const std::size_t          count,
                wasm_trap_t**              trapOutput
            ):
                operation{ std::move(asyncOperation) },
                resultKinds{ std::move(kinds) },
                results{ resultStorage },
                resultCount{ count },
                trapRet{ trapOutput }
            {}
        };

        bool PollHostContinuation(void* environment) noexcept
        {
            auto* state = static_cast<ContinuationState*>(environment);

            try
            {
                auto pollResult = state->operation.Poll();
                if (!pollResult)
                {
                    SetTrap(state->trapRet, pollResult.GetError().Message());
                    return true;
                }

                if (pollResult.Value() == AsyncPollStatus::Pending)
                {
                    return false;
                }

                auto result = state->operation.TakeResult();
                if (!result)
                {
                    SetTrap(state->trapRet, result.GetError().Message());
                    return true;
                }

                const auto& values = result.Value();
                if (
                    values.size() != state->resultKinds.size() ||
                    values.size() != state->resultCount
                )
                {
                    SetTrap(
                        state->trapRet,
                        "Async host operation returned "
                        "the wrong number of results."
                    );
                    return true;
                }

                for (std::size_t i = 0; i < values.size(); ++i)
                {
                    if (!WriteHostValue(
                        state->resultKinds[i],
                        values[i],
                        state->results[i]
                    ))
                    {
                        SetTrap(
                            state->trapRet,
                            "Async host operation returned "
                            "a value with the wrong type."
                        );
                        return true;
                    }
                }

                return true;
            }
            catch(const std::exception& exception)
            {
                SetTrap(state->trapRet, exception.what());
                return true;
            }
            catch(...)
            {
                SetTrap(
                    state->trapRet,
                    "Unknown exception escaped from "
                    "an asynchronous host operation."
                );
                return true;
            }
        }

        void DeleteContinuationState(void* environment)
        {
            delete static_cast<ContinuationState*>(environment);
        }

        void DeleteAsyncHostBinding(void* environment)
        {
            delete static_cast<AsyncHostBinding*>(environment);
        }

        void AsyncHostCallback(
            void*                          environment,
            wasmtime_caller_t*,
            const wasmtime_val_t*          args,
            const std::size_t              nargs,
            wasmtime_val_t*                results,
            const std::size_t              nresults,
            wasm_trap_t**                  trapRet,
            wasmtime_async_continuation_t* continuationRet
        )
        {
            auto* binding = static_cast<AsyncHostBinding*>(environment);

            try
            {
                if (
                    nargs != binding->signature.parameters.size() ||
                    nresults != binding->signature.results.size()
                )
                {
                    SetTrap(
                        trapRet,
                        "Wasmtime async host invocation "
                        "signature mismatch."
                    );
                    ConfigureImmediateContinuation(continuationRet);

                    return;
                }

                std::vector<HostValue> arguments;
                arguments.reserve(nargs);

                for (std::size_t i = 0; i < nargs; ++i)
                {
                    HostValue value{ std::int32_t{ 0 } };

                    if (!ReadHostValue(binding->signature.parameters[i], args[i], value))
                    {
                        SetTrap(
                            trapRet,
                            "Wasmtime async host argument "
                            "type mismatch."
                        );
                        ConfigureImmediateContinuation(continuationRet);

                        return;
                    }

                    arguments.push_back(std::move(value));
                }

                auto operationResult = binding->function(arguments);
                if (!operationResult)
                {
                    SetTrap(trapRet, operationResult.GetError().Message());
                    ConfigureImmediateContinuation(continuationRet);

                    return;
                }

                auto* continuationState = new ContinuationState{
                    std::move(operationResult).Value(),
                    binding->signature.results,
                    results,
                    nresults,
                    trapRet
                };

                continuationRet->callback = &PollHostContinuation;
                continuationRet->env = continuationState;
                continuationRet->finalizer = &DeleteContinuationState;
            }
            catch(const std::exception& exception)
            {
                SetTrap(trapRet, exception.what());
                ConfigureImmediateContinuation(continuationRet);
            }
            catch(...)
            {
                SetTrap(
                    trapRet,
                    "Unknown exception escaped from "
                    "an asynchronous host callback."
                );
                ConfigureImmediateContinuation(continuationRet);
            }
        }
    }

    Result<void> ExecutionDomain::DefineAsyncFunction(
        const std::string_view            module,
        const std::string_view            name,
        const AsyncHostFunctionSignature& signature,
        AsyncHostFunction                 function
    )
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "ExecutionDomain is in "
                "a moved-from state."
            };
        }

        if (!function)
        {
            return Error{
                ErrorCode::AsyncHostFunctionDefinitionFailed,
                "Async host function callback "
                "cannot be empty."
            };
        }

        auto leaseResult = m_pState->executionState.Acquire(
            detail::StoreExecutionOperation::ExclusiveMutation,
            "async host function definition"
        );
        if (!leaseResult)
        {
            return leaseResult.GetError();
        }

        auto executionLease = std::move(leaseResult).Value();

        std::unique_ptr<wasm_functype_t, decltype(&wasm_functype_delete)> functionType{
            CreateFunctionType(signature),
            &wasm_functype_delete
        };

        if (!functionType)
        {
            return Error{
                ErrorCode::AsyncHostFunctionDefinitionFailed,
                "Failed to create Wasmtime "
                "function type."
            };
        }

        AsyncHostBinding* binding = nullptr;

        try
        {
            binding = new AsyncHostBinding{
                signature,
                std::move(function)
            };
        }
        catch(const std::exception& exception)
        {
            return Error{
                ErrorCode::AsyncHostFunctionDefinitionFailed,
                exception.what()
            };
        }

        wasmtime_error_t* error = wasmtime_linker_define_async_func(
            m_pState->linker.get(),
            module.data(),
            module.size(),
            name.data(),
            name.size(),
            functionType.get(),
            &AsyncHostCallback,
            binding,
            &DeleteAsyncHostBinding
        );

        if (error != nullptr)
        {
            return Error{
                ErrorCode::AsyncHostFunctionDefinitionFailed,
                detail::TakeWasmtimeError(error)
            };
        }

        m_pState->RegisterAsyncHostFunction(module, name);

        return {};
    }
}
