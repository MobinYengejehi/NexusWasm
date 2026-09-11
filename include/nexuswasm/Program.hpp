#pragma once

#ifndef NEXUSWASM_PROGRAM_HEADER
#define NEXUSWASM_PROGRAM_HEADER

#include <memory>
#include <string_view>

#include <nexuswasm/ExecutionDomain.hpp>
#include <nexuswasm/Export.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct RealmState;
        class  ModuleGraph;
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

        [[nodiscard]]
        Result<void> AddModule(
            std::string_view moduleNamespace,
            const Module&    module
        );

        [[nodiscard]]
        Result<void> Instantiate();

        [[nodiscard]]
        Result<Instance> GetInstance(std::string_view moduleNamespace) const;

    private:
        std::shared_ptr<detail::RealmState> m_pRealm;

        ExecutionDomain m_cMainDomain;

        std::unique_ptr<detail::ModuleGraph> m_pModuleGraph;

        Program(
            std::shared_ptr<detail::RealmState> realmState,
            ExecutionDomain                     mainDomain
        );

        friend class Realm;
    };
}

#endif
