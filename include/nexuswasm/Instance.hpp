#pragma once

#ifndef NEXUSWASM_INSTANCE_HEADER
#define NEXUSWASM_INSTANCE_HEADER

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include <nexuswasm/AsyncCall.hpp>
#include <nexuswasm/Export.hpp>
#include <nexuswasm/Result.hpp>
#include <nexuswasm/detail/Scaler.hpp>

namespace nexus
{
    namespace detail
    {
        struct InstanceState;
        struct AsyncCallState;
        class  ModuleGraph;
    }

    class ExecutionDomain;
    class AsyncInstantiation;

    class NEXUSWASM_API Instance final
    {
    public:
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&&) noexcept;
        Instance& operator=(Instance&&) noexcept;

        ~Instance();

        template<typename ResultType, typename... Args>
        [[nodiscard]]
        Result<ResultType> Call(
            const std::string_view name,
            Args...                args
        )
        {
            static_assert(
                detail::IsSupportedScaler<ResultType>,
                "NexusWasm currently supports only i32, i64, f32 and f64 results"
            );

            static_assert(
                (detail::IsSupportedScaler<std::decay_t<Args>> && ...),
                "NexusWasm currently supports only i32, i64, f32 and f64 parameters"
            );

            const std::array<detail::ScalerValue, sizeof...(Args)> packedArgs{ detail::PackScaler(args)... };

            auto result = CallScaler(
                name,
                packedArgs.data(),
                packedArgs.size(),
                detail::ScalerKindOf<ResultType>()
            );

            if (!result)
            {
                return result.GetError();
            }

            return detail::UnpackScaler<ResultType>(result.Value());
        }

        template<typename ResultType, typename... Args>
        [[nodiscard]]
        Result<AsyncCall<ResultType>> CallAsync(
            const std::string_view name,
            Args...                args
        )
        {
            static_assert(
                detail::IsSupportedScaler<ResultType>,
                "NexusWasm currently supports only "
                "i32, i64, f32 and f64 async results"
            );

            static_assert(
                (detail::IsSupportedScaler<std::decay_t<Args>> && ...),
                "NexusWasm currently supports only "
                "i32, i64, f32 and f64 async parameters"
            );

            const std::array<detail::ScalerValue, sizeof...(Args)> packedArgs{ detail::PackScaler(args)... };

            auto stateResult = CallScalerAsync(name, packedArgs.data(), packedArgs.size(), detail::ScalerKindOf<ResultType>());
            if (!stateResult)
            {
                return stateResult.GetError();
            }

            return AsyncCall<ResultType>{ std::move(stateResult).Value() };
        }

    private:
        std::unique_ptr<detail::InstanceState> m_pState;

        explicit Instance(std::unique_ptr<detail::InstanceState> state) noexcept;

        [[nodiscard]]
        Result<detail::ScalerValue> CallScaler(
            std::string_view           name,
            const detail::ScalerValue* args,
            std::size_t                argumentCount,
            detail::ScalerKind         expectedResult
        );

        [[nodiscard]]
        Result<std::shared_ptr<detail::AsyncCallState>> CallScalerAsync(
            std::string_view           name,
            const detail::ScalerValue* args,
            std::size_t                argumentCount,
            detail::ScalerKind         expectedResult
        );

        friend class ExecutionDomain;
        friend class AsyncInstantiation;
        friend class detail::ModuleGraph;
    };
}

#endif
