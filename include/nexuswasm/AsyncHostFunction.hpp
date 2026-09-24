#pragma once

#ifndef NEXUSWASM_ASYNC_HOST_FUNCTION_HEADER
#define NEXUSWASM_ASYNC_HOST_FUNCTION_HEADER

#include <cstdint>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

#include <nexuswasm/AsyncCall.hpp>
#include <nexuswasm/Error.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    enum class HostValueKind: std::uint8_t
    {
        I32,
        I64,
        F32,
        F64
    };

    using HostValue = std::variant<
        std::int32_t,
        std::int64_t,
        float,
        double
    >;

    struct AsyncHostFunctionSignature final
    {
        std::vector<HostValueKind> parameters;
        std::vector<HostValueKind> results;
    };

    class AsyncHostOperation final
    {
    public:
        using PollFunction = std::function<Result<AsyncPollStatus>()>;
        using TakeResultFunction = std::function<Result<std::vector<HostValue>>()>;

        AsyncHostOperation(
            PollFunction       pollFunction,
            TakeResultFunction takeResultFunction
        ):
            m_pPollFunction{ std::move(pollFunction) },
            m_pTakeResultFunction{ std::move(takeResultFunction) }
        {}

        [[nodiscard]]
        Result<AsyncPollStatus> Poll()
        {
            if (!m_pPollFunction)
            {
                return Error{
                    ErrorCode::InvalidState,
                    "Async host operation has "
                    "no poll function."
                };
            }

            return m_pPollFunction();
        }

        [[nodiscard]]
        Result<std::vector<HostValue>> TakeResult()
        {
            if (!m_pTakeResultFunction)
            {
                return Error{
                    ErrorCode::InvalidState,
                    "Async host operation has "
                    "no result function."
                };
            }

            return m_pTakeResultFunction();
        }

    private:
        PollFunction       m_pPollFunction;
        TakeResultFunction m_pTakeResultFunction;
    };

    using AsyncHostFunction = std::function<Result<AsyncHostOperation>(const std::vector<HostValue>&)>;
}

#endif
