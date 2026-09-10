#pragma once

#ifndef NEXUSWASM_RUNTIME_HEADER
#define NEXUSWASM_RUNTIME_HEADER

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <nexuswasm/Module.hpp>
#include <nexuswasm/Export.hpp>
#include <nexuswasm/Realm.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct EngineState;
    }

    class NEXUSWASM_API Runtime final
    {
    public:
        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        Runtime(Runtime&&) noexcept;
        Runtime& operator=(Runtime&&) noexcept;

        ~Runtime();

        [[nodiscard]]
        Result<Realm> CreateRealm() const;

        [[nodiscard]]
        Result<Module> Compile(const std::uint8_t* data, std::size_t size) const;

        [[nodiscard]]
        Result<Module> Compile(const std::vector<std::uint8_t>& bytes) const;

        [[nodiscard]]
        static Result<Runtime> Create();

    private:
        explicit Runtime(std::shared_ptr<detail::EngineState> state) noexcept;

        std::shared_ptr<detail::EngineState> m_pState;
    };
}

#endif
