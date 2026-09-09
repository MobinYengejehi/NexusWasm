#include <nexuswasm/Instance.hpp>

#include <string>
#include <utility>
#include <vector>

#include <wasmtime.h>

#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    namespace
    {
        std::string TakeWasmtimeError(wasmtime_error_t* error)
        {
            if (error == nullptr)
            {
                return {};
            }

            wasm_name_t message;
            wasmtime_error_message(error, &message);

            std::string result{ message.data, message.size };

            wasm_byte_vec_delete(&message);
            wasmtime_error_delete(error);

            if (!result.empty() && result.back() == '\0')
            {
                result.pop_back();
            }

            return result;
        }

        std::string TakeTrap(wasm_trap_t* trap)
        {
            if (trap == nullptr)
            {
                return {};
            }

            wasm_message_t message{};
            wasm_trap_message(trap, &message);

            std::string result{ message.data, message.size };

            wasm_byte_vec_delete(&message);
            wasm_trap_delete(trap);

            if (!result.empty() && result.back() == '\0')
            {
                result.pop_back();
            }

            return result;
        }

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

        wasmtime_context_t* context = wasmtime_store_context(m_pState->store.get());

        wasmtime_extern_t exportItem{};

        const bool found = wasmtime_instance_export_get(
            context,
            &m_pState->instance,
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

        struct ExternGuard final
        {
            wasmtime_extern_t* value;

            ~ExternGuard()
            {
                wasmtime_extern_delete(value);
            }
        };

        ExternGuard exportGuard{ &exportItem };

        if (exportItem.kind != WASMTIME_EXTERN_FUNC)
        {
            return Error{
                ErrorCode::ExportNotFunction,
                "WebAssembly export is not a function"
            };
        }

        wasm_functype_t* functionType = wasmtime_func_type(context, &exportItem.of.func);
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
                "NexusWasm basic call currently requires exactly one result."
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
                    "WebAssembly function parameter type mismatch."
                };
            }
        }

        if (wasm_valtype_kind(results->data[0]) != ToWasmKind(expectedResult))
        {
            return Error{
                ErrorCode::SignatureMismatch,
                "WebAssembly function result type mismatch."
            };
        }

        std::vector<wasmtime_val_t> wasmtimeArguments;
        wasmtimeArguments.reserve(argumentCount);

        for (std::size_t i = 0; i < argumentCount; ++i)
        {
            wasmtimeArguments.push_back(ToWasmtimeValue(args[i]));
        }

        wasmtime_val_t    result{};
        wasm_trap_t*      trap = nullptr;
        wasmtime_error_t* error = wasmtime_func_call(
            context,
            &exportItem.of.func,
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
                TakeWasmtimeError(error)
            };
        }
        if (trap != nullptr)
        {
            return Error{
                ErrorCode::Trap,
                TakeTrap(trap)
            };
        }

        return FromWasmtimeValue(result);
    }
}
