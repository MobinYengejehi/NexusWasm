#include <nexuswasm/Program.hpp>

#include <utility>

#include "module/ModuleGraph.hpp"
#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    Program::Program(
        std::shared_ptr<detail::RealmState> realmState,
        ExecutionDomain                     mainDomain
    ):
        m_pRealm{ std::move(realmState) },
        m_cMainDomain{ std::move(mainDomain) },
        m_pModuleGraph{ std::make_unique<detail::ModuleGraph>() }
    {}

    Program::~Program() = default;

    Program::Program(Program&&) noexcept = default;
    Program& Program::operator=(Program&&) noexcept = default;

    ExecutionDomain& Program::MainDomain() noexcept
    {
        return m_cMainDomain;
    }

    const ExecutionDomain& Program::MainDomain() const noexcept
    {
        return m_cMainDomain;
    }

    Result<void> Program::AddModule(
        const std::string_view moduleNamespace,
        const Module&          module
    )
    {
        if (!m_pRealm)
        {
            return Error{
                ErrorCode::InvalidState,
                "Program is in a moved-from state."
            };
        }

        return m_pModuleGraph->AddModule(m_pRealm->engine, moduleNamespace, module);
    }

    Result<void> Program::Instantiate()
    {
        if (!m_pRealm)
        {
            return Error{
                ErrorCode::InvalidState,
                "Program is in a moved-from state."
            };
        }

        return m_pModuleGraph->Instantiate(m_cMainDomain.m_pState);
    }

    Result<Instance> Program::GetInstance(const std::string_view moduleNamespace) const
    {
        if (!m_pRealm)
        {
            return Error{
                ErrorCode::InvalidState,
                "Program is in a moved-from state."
            };
        }

        return m_pModuleGraph->GetInstance(m_cMainDomain.m_pState, moduleNamespace);
    }
}
