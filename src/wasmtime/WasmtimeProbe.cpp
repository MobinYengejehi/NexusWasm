#include "wasmtime/WasmtimeProbe.hpp"

#include <memory>
#include <string>

#include <wasmtime.h>

namespace nexus::detail
{
    namespace
    {
        struct EngineDeleter final
        {
            void operator()(wasm_engine_t* engine) const noexcept
            {
                wasm_engine_delete(engine);
            }
        };

        struct StoreDeleter final
        {
            void operator()(wasmtime_store_t* store) const noexcept
            {
                wasmtime_store_delete(store);
            }
        };

        struct ModuleDeleter final
        {
            void operator()(wasmtime_module_t* module) const noexcept
            {
                wasmtime_module_delete(module);
            }
        };

        using EnginePtr = std::unique_ptr<wasm_engine_t, EngineDeleter>;
        using StorePtr = std::unique_ptr<wasmtime_store_t, StoreDeleter>;
        using ModulePtr = std::unique_ptr<wasmtime_module_t, ModuleDeleter>;

        std::string TakeErrorMessage(wasmtime_error_t* error)
        {
            if (error == nullptr)
            {
                return {};
            }

            wasm_name_t message{};
            wasmtime_error_message(error, &message);

            std::string result(message.data, message.size);

            wasm_byte_vec_delete(&message);
            wasmtime_error_delete(error);

            if (!result.empty() && result.back() == '\0')
            {
                result.pop_back();
            }

            return result;
        }

        std::string TakeTrapMessage(wasm_trap_t* trap)
        {
            if (trap == nullptr)
            {
                return {};
            }

            wasm_message_t message{};
            wasm_trap_message(trap, &message);

            std::string result(message.data, message.size);

            wasm_byte_vec_delete(&message);
            wasm_trap_delete(trap);

            if (!result.empty() && result.back() == '\0')
            {
                result.pop_back();
            }

            return result;
        }

        WasmtimeProbeResult Failure(const WasmtimeProbeFailure failure, std::string message)
        {
            WasmtimeProbeResult result;

            result.success = false;
            result.failure = failure;
            result.message = std::move(message);

            return result;
        }
    }

    WasmtimeProbeResult RunWasmtimeI32Binary(
        const std::string_view wat,
        const std::string_view exportName,
        const std::int32_t     lhs,
        const std::int32_t     rhs
    )
    {
        EnginePtr engine{ wasm_engine_new() };
        if (!engine)
        {
            return Failure(WasmtimeProbeFailure::EngineCreation, "wasm_engine_new failed");
        }

        wasm_byte_vec_t   wasmBytes{};
        wasmtime_error_t* watError = wasmtime_wat2wasm(wat.data(), wat.size(), &wasmBytes);

        if (watError != nullptr)
        {
            return Failure(WasmtimeProbeFailure::WatParsing, TakeErrorMessage(watError));
        }

        struct WasmBytesGuard final
        {
            wasm_byte_vec_t* bytes;

            ~WasmBytesGuard()
            {
                wasm_byte_vec_delete(bytes);
            }
        };

        WasmBytesGuard wasmBytesGuard{ &wasmBytes };

        wasmtime_module_t* rawModule = nullptr;
        wasmtime_error_t*  moduleError = wasmtime_module_new(
            engine.get(),
            reinterpret_cast<const std::uint8_t*>(wasmBytes.data),
            wasmBytes.size,
            &rawModule
        );

        if (moduleError != nullptr)
        {
            return Failure(WasmtimeProbeFailure::ModuleCompilation, TakeErrorMessage(moduleError));
        }
        if (rawModule == nullptr)
        {
            return Failure(WasmtimeProbeFailure::ModuleCompilation, "wasmtime_module_new returned no module");
        }

        ModulePtr module{ rawModule };

        StorePtr store{ wasmtime_store_new(engine.get(), nullptr, nullptr) };
        if (!store)
        {
            return Failure(WasmtimeProbeFailure::StoreCreation, "wasmtime_store_new failed");
        }

        wasmtime_context_t* context = wasmtime_store_context(store.get());

        wasmtime_instance_t instance{};
        wasm_trap_t*        instantiateTrap = nullptr;
        wasmtime_error_t*   instantiateError = wasmtime_instance_new(
            context,
            module.get(),
            nullptr,
            0,
            &instance,
            &instantiateTrap
        );

        if (instantiateError != nullptr)
        {
            return Failure(WasmtimeProbeFailure::Instantiation, TakeErrorMessage(instantiateError));
        }
        if (instantiateTrap != nullptr)
        {
            return Failure(WasmtimeProbeFailure::Instantiation, TakeTrapMessage(instantiateTrap));
        }

        wasmtime_extern_t exportItem{};
        const bool        found = wasmtime_instance_export_get(
            context,
            &instance,
            exportName.data(),
            exportName.size(),
            &exportItem
        );

        if (!found)
        {
            return Failure(WasmtimeProbeFailure::ExportNotFound, "WebAssembly export was not found");
        }

        struct ExternGuard final
        {
            wasmtime_extern_t* item;

            ~ExternGuard()
            {
                wasmtime_extern_delete(item);
            }
        };

        ExternGuard exportGuard{ &exportItem };

        if (exportItem.kind != WASMTIME_EXTERN_FUNC)
        {
            return Failure(WasmtimeProbeFailure::ExportNotFound, "WebAssembly export is not a function");
        }

        wasmtime_val_t arguments[2]{};

        arguments[0].kind = WASMTIME_I32;
        arguments[0].of.i32 = lhs;

        arguments[1].kind = WASMTIME_I32;
        arguments[1].of.i32 = rhs;

        wasmtime_val_t results[1]{};

        wasm_trap_t*      callTrap = nullptr;
        wasmtime_error_t* callError = wasmtime_func_call(
            context,
            &exportItem.of.func,
            arguments,
            2,
            results,
            1,
            &callTrap
        );

        if (callError != nullptr)
        {
            return Failure(WasmtimeProbeFailure::CallError, TakeErrorMessage(callError));
        }
        if (callTrap != nullptr)
        {
            return Failure(WasmtimeProbeFailure::Trap, TakeTrapMessage(callTrap));
        }

        if (results[0].kind != WASMTIME_I32)
        {
            return Failure(WasmtimeProbeFailure::InvalidResult, "Expected an i32 WebAssembly result");
        }

        WasmtimeProbeResult result;

        result.success = true;
        result.value = results[0].of.i32;
        result.failure = WasmtimeProbeFailure::None;

        return result;
    }
}
