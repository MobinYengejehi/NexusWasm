#include <nexuswasm/Realm.hpp>

#include <utility>

#include "wasmtime/WasmtimeState.hpp"

namespace nexus
{
    Realm::Realm(std::shared_ptr<detail::RealmState> state) noexcept:
        m_pState(state)
    {}

    Realm::~Realm() = default;

    Realm::Realm(Realm&&) noexcept = default;
    Realm& Realm::operator=(Realm&&) noexcept = default;

    Result<Program> Realm::CreateProgram() const
    {
        if (!m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Realm is in a moved-from state."
            };
        }

        auto domainResult = ExecutionDomain::Create(m_pState->engine);
        if (!domainResult)
        {
            return domainResult.GetError();
        }

        return Program{
            m_pState,
            std::move(domainResult).Value()
        };
    }
}
