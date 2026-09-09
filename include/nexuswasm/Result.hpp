#pragma once

#ifndef NEXUSWASM_RESULT_HEADER
#define NEXUSWASM_RESULT_HEADER

#include <cassert>
#include <utility>
#include <variant>

#include <nexuswasm/Error.hpp>

namespace nexus
{
    template<typename T>
    class [[nodiscard]] Result final
    {
    public:
        Result(const T& value):
            m_vStorage{ value }
        {}

        Result(T&& value):
            m_vStorage{ std::move(value) }
        {}

        Result(const Error& error):
            m_vStorage{ error }
        {}

        Result(Error&& error):
            m_vStorage{ std::move(error) }
        {}

        [[nodiscard]]
        bool HasValue() const noexcept
        {
            return std::holds_alternative<T>(m_vStorage);
        }

        [[nodiscard]]
        explicit operator bool() const noexcept
        {
            return HasValue();
        }

        [[nodiscard]]
        T& Value() &
        {
            assert(HasValue());
            return *std::get_if<T>(&m_vStorage);
        }

        [[nodiscard]]
        const T& Value() const&
        {
            assert(HasValue());
            return *std::get_if<T>(&m_vStorage);
        }

        [[nodiscard]]
        T&& Value() &&
        {
            assert(HasValue());
            return std::move(*std::get_if<T>(&m_vStorage));
        }

        [[nodiscard]]
        Error& GetError() &
        {
            assert(!HasValue());
            return *std::get_if<Error>(&m_vStorage);
        }

        [[nodiscard]]
        const Error& GetError() const &
        {
            assert(!HasValue());
            return *std::get_if<Error>(&m_vStorage);
        }

    private:
        std::variant<T, Error> m_vStorage;
    };
}

#endif
