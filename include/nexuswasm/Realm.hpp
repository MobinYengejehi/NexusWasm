#pragma once

#ifndef NEXUSWASM_REALM_HEADER
#define NEXUSWASM_REALM_HEADER

#include <memory>

#include <nexuswasm/Export.hpp>
#include <nexuswasm/Program.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct RealmState;
    }

    class Runtime;

    class NEXUSWASM_API Realm final
    {
    public:
        Realm(const Realm&) = delete;
        Realm& operator=(const Realm&) = delete;

        Realm(Realm&&) noexcept;
        Realm& operator=(Realm&&) noexcept;

        ~Realm();

        [[nodiscard]]
        Result<Program> CreateProgram() const;

    private:
        std::shared_ptr<detail::RealmState> m_pState;

        explicit Realm(std::shared_ptr<detail::RealmState> state) noexcept;

        friend class Runtime;
    };
}

#endif
