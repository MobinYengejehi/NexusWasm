#pragma once

#ifndef NEXUSWASM_ASYNC_CALL_HEADER
#define NEXUSWASM_ASYNC_CALL_HEADER

#include <cstdint>
#include <memory>
#include <utility>

#include <nexuswasm/Export.hpp>
#include <nexuswasm/Result.hpp>
#include <nexuswasm/detail/Scaler.hpp>

namespace nexus
{
    enum class AsyncPollStatus: std::uint8_t
    {
        Pending,
        Ready
    };

    namespace detail
    {
        struct AsyncCallState;

        [[nodiscard]]
        NEXUSWASM_API Result<AsyncPollStatus> PollAsyncCall(
            const std::shared_ptr<AsyncCallState>& state
        );

        [[nodiscard]]
        NEXUSWASM_API Result<ScalerValue> TakeAsyncCallResult(
            const std::shared_ptr<AsyncCallState>& state
        );
    }

    class Instance;

    template<typename ResultType>
    class [[nodiscard]] AsyncCall final
    {
    public:
        static_assert(
            detail::IsSupportedScaler<ResultType>,
            "NexusWasm currently supports only "
            "i32, i64, f32 and f64 async results"
        );

        AsyncCall(const AsyncCall&) = delete;
        AsyncCall& operator=(const AsyncCall&) = delete;

        AsyncCall(AsyncCall&&) noexcept = default;
        AsyncCall& operator=(AsyncCall&&) noexcept = default;

        ~AsyncCall() = default;

        [[nodiscard]]
        Result<AsyncPollStatus> Poll()
        {
            return detail::PollAsyncCall(m_pState);
        }

        [[nodiscard]]
        Result<ResultType> TakeResult()
        {
            auto result = detail::TakeAsyncCallResult(m_pState);
            if (!result)
            {
                return result.GetError();
            }

            return detail::UnpackScaler<ResultType>(result.Value());
        }

        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return static_cast<bool>(m_pState);
        }

    private:
        std::shared_ptr<detail::AsyncCallState> m_pState;

        explicit AsyncCall(std::shared_ptr<detail::AsyncCallState> state) noexcept:
            m_pState{ std::move(state) }
        {}

        friend class Instance;
    };
}

#endif
