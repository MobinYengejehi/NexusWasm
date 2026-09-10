#include <nexuswasm/Program.hpp>

#include <utility>

namespace nexus
{
    Program::Program(
        std::shared_ptr<detail::RealmState> realmState,
        ExecutionDomain                     mainDomain
    ) noexcept:
        m_pRealm{ std::move(realmState) },
        m_cMainDomain{ std::move(mainDomain) }
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
}
