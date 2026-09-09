#include <nexuswasm/Runtime.hpp>

#include <memory>
#include <string>
#include <utility>

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
    }

    Runtime::Runtime(std::shared_ptr<detail::EngineState> state) noexcept:
        m_pState{ std::move(state) }
    {}

    Runtime::~Runtime() = default;

    Runtime::Runtime(Runtime&&) noexcept = default;
    Runtime& Runtime::operator=(Runtime&&) noexcept = default;

    Result<Runtime> Runtime::Create()
    {
        wasm_engine_t* engine = wasm_engine_new();
        if (engine == nullptr)
        {
            return Error{
                ErrorCode::EngineCreationFailed,
                "Failed to create Wasmtime engine."
            };
        }

        auto state = std::make_shared<detail::EngineState>(engine);

        return Runtime{ std::move(state) };
    }

    Result<CompiledModule> Runtime::Compile(const std::uint8_t* data, const std::size_t size) const
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Runtime is in a moved-from state."
            };
        }

        if (data == nullptr && size != 0)
        {
            return Error{
                ErrorCode::CompilationFailed,
                "WebAssembly data pointer is null"
            };
        }

        wasmtime_module_t* rawModule = nullptr;
        wasmtime_error_t*  error = wasmtime_module_new(m_pState->engine.get(), data, size, &rawModule);

        if (error != nullptr)
        {
            return Error{
                ErrorCode::CompilationFailed,
                TakeWasmtimeError(error)
            };
        }
        if (rawModule == nullptr)
        {
            return Error{
                ErrorCode::CompilationFailed,
                "Wasmtime returned no compiled module."
            };
        }

        auto moduleState = std::make_shared<detail::ModuleState>(m_pState, rawModule);

        return CompiledModule{ std::move(moduleState) };
    }

    Result<CompiledModule> Runtime::Compile(const std::vector<std::uint8_t>& bytes) const
    {
        return Compile(bytes.data(), bytes.size());
    }

    Result<Instance> Runtime::Instantiate(const CompiledModule& module) const
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Runtime is in a moved-from state."
            };
        }

        if (!module.m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "CompiledModule is in a moved-from state."
            };
        }

        if (module.m_pState->engine.get() != m_pState.get())
        {
            return Error{
                ErrorCode::RuntimeMismatch,
                "CompiledModule belongs to another Runtime."
            };
        }

        wasmtime_store_t* rawStore = wasmtime_store_new(m_pState->engine.get(), nullptr, nullptr);
        if (rawStore == nullptr)
        {
            return Error{
                ErrorCode::InstantiationFailed,
                "Failed to create Wasmtime store"
            };
        }

        detail::StorePtr store{ rawStore };

        wasmtime_context_t* context = wasmtime_store_context(store.get());

        wasmtime_instance_t instance{};
        wasm_trap_t*        trap = nullptr;
        wasmtime_error_t*   error = wasmtime_instance_new(
            context,
            module.m_pState->module.get(),
            nullptr,
            0,
            &instance,
            &trap
        );

        if (error != nullptr)
        {
            return Error{
                ErrorCode::InstantiationFailed,
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

        auto instanceState = std::make_unique<detail::InstanceState>(m_pState, store.release(), instance);

        return Instance{ std::move(instanceState) };
    }
}
