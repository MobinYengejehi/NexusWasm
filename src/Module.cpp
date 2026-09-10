#include <nexuswasm/Module.hpp>

#include <utility>

#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    Module::Module(std::shared_ptr<detail::ModuleState> state) noexcept:
        m_pState{ std::move(state) }
    {}

    Module::~Module() = default;

    Module::Module(const Module&) noexcept = default;
    Module& Module::operator=(const Module&) noexcept = default;

    Module::Module(Module&&) noexcept = default;
    Module& Module::operator=(Module&&) noexcept = default;
}
