#pragma once

#ifndef NEXUSWASM_PROGRAM_HEADER
#define NEXUSWASM_PROGRAM_HEADER

#include <memory>

#include <nexuswasm/ExecutionDomain.hpp>
#include <nexuswasm/Export.hpp>

namespace nexus
{
    namespace detail
    {
        struct RealmState;
    }

    class Realm;

    class NEXUSWASM_API Program final
    {
    public:
        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

        Program(Program&&) noexcept;
        Program& operator=(Program&&) noexcept;

        ~Program();

        [[nodiscard]]
        ExecutionDomain& MainDomain() noexcept;

        [[nodiscard]]
        const ExecutionDomain& MainDomain() const noexcept;

    private:
        std::shared_ptr<detail::RealmState> m_pRealm;

        ExecutionDomain m_cMainDomain;

        Program(
            std::shared_ptr<detail::RealmState> realmState,
            ExecutionDomain                     mainDomain
        ) noexcept;

        friend class Realm;
    };
}

#endif
