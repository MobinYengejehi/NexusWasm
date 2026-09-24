#pragma once

#ifndef NEXUSWASM_ASYNC_INSTANTIATION_HEADER
#define NEXUSWASM_ASYNC_INSTANTIATION_HEADER

#include <memory>

#include <nexuswasm/AsyncCall.hpp>
#include <nexuswasm/Export.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct AsyncInstantiationState;
    }

    class ExecutionDomain;

    class NEXUSWASM_API AsyncInstantiation final
    {
    public:
        AsyncInstantiation(const AsyncInstantiation&) = delete;
        AsyncInstantiation& operator=(const AsyncInstantiation&) = delete;

        AsyncInstantiation(AsyncInstantiation&&) noexcept = default;
        AsyncInstantiation& operator=(AsyncInstantiation&&) noexcept = default;

        ~AsyncInstantiation() = default;

        [[nodiscard]]
        Result<AsyncPollStatus> Poll();

        [[nodiscard]]
        Result<Instance> TakeInstance();

    private:
        std::shared_ptr<detail::AsyncInstantiationState> m_pState;

        explicit AsyncInstantiation(std::shared_ptr<detail::AsyncInstantiationState> state) noexcept:
            m_pState{ std::move(state) }
        {}

        friend class ExecutionDomain;
    };
}

#endif
