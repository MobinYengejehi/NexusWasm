#include <nexuswasm/CompiledModule.hpp>

#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    CompiledModule::CompiledModule(std::shared_ptr<detail::ModuleState> state) noexcept:
        m_pState{ std::move(state) }
    {}

    CompiledModule::~CompiledModule() = default;
    
    CompiledModule::CompiledModule(const CompiledModule&) noexcept = default;
    CompiledModule& CompiledModule::operator=(const CompiledModule&) noexcept = default;

    CompiledModule::CompiledModule(CompiledModule&&) noexcept = default;
    CompiledModule& CompiledModule::operator=(CompiledModule&&) noexcept = default;
}
