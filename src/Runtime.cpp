#include <nexuswasm/Runtime.hpp>

#include <memory>
#include <string>
#include <utility>

#include <wasmtime.h>

#include "wasmtime/WasmtimeState.hpp"
#include "wasmtime/WasmtimeError.hpp"

#include "module/ModuleMetadata.hpp"

namespace nexus
{
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

    Result<Realm> Runtime::CreateRealm() const
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Runtime is in a moved-from state."
            };
        }

        auto realmState = std::make_shared<detail::RealmState>(m_pState);

        return Realm{ std::move(realmState) };
    }

    Result<Module> Runtime::Compile(const std::uint8_t* data, const std::size_t size) const
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
                detail::TakeWasmtimeError(error)
            };
        }
        if (rawModule == nullptr)
        {
            return Error{
                ErrorCode::CompilationFailed,
                "Wasmtime returned no compiled module."
            };
        }

        detail::ModulePtr module{ rawModule };

        auto metadata = detail::InspectModule(module.get());
        auto moduleState = std::make_shared<detail::ModuleState>(m_pState, module.release(), std::move(metadata));

        return Module{ std::move(moduleState) };
    }

    Result<Module> Runtime::Compile(const std::vector<std::uint8_t>& bytes) const
    {
        return Compile(bytes.data(), bytes.size());
    }
}
