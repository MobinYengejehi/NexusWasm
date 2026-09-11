#pragma once

#ifndef NEXUSWASM_INSTANCE_HEADER
#define NEXUSWASM_INSTANCE_HEADER

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>

#include <nexuswasm/Export.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus
{
    namespace detail
    {
        struct InstanceState;
        class  ModuleGraph;

        enum class ScalerKind: std::uint8_t
        {
            I32,
            I64,
            F32,
            F64
        };

        struct ScalerValue final
        {
            ScalerKind kind;

            union
            {
                std::int32_t i32;
                std::int64_t i64;

                float  f32;
                double f64;
            };
        };

        template<typename T>
        inline constexpr bool IsSupportedScaler = (
            std::is_same_v<T, std::int32_t> ||
            std::is_same_v<T, std::int64_t> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, double>
        );

        template<typename T>
        constexpr ScalerKind ScalerKindOf()
        {
            static_assert(IsSupportedScaler<T>, "Unsupported NexusWasm scaler type");

            if constexpr(std::is_same_v<T, std::int32_t>)
            {
                return ScalerKind::I32;
            }
            else if constexpr(std::is_same_v<T, std::int64_t>)
            {
                return ScalerKind::I64;
            }
            else if constexpr(std::is_same_v<T, float>)
            {
                return ScalerKind::F32;
            }
            else
            {
                return ScalerKind::F64;
            }
        }

        template<typename T>
        ScalerValue PackScaler(const T value)
        {
            using Type = std::decay_t<T>;
            static_assert(IsSupportedScaler<Type>, "Unsupported NexusWasm scaler argument");

            ScalerValue result{};
            result.kind = ScalerKindOf<Type>();

            if constexpr(std::is_same_v<Type, std::int32_t>)
            {
                result.i32 = value;
            }
            else if constexpr(std::is_same_v<Type, std::int64_t>)
            {
                result.i64 = value;
            }
            else if constexpr(std::is_same_v<Type, float>)
            {
                result.f32 = value;
            }
            else
            {
                result.f64 = value;
            }

            return result;
        }

        template<typename T>
        T UnpackScaler(const ScalerValue& value)
        {
            static_assert(IsSupportedScaler<T>, "Unsupported NexusWasm scaler result");

            if constexpr(std::is_same_v<T, std::int32_t>)
            {
                return value.i32;
            }
            else if constexpr(std::is_same_v<T, std::int64_t>)
            {
                return value.i64;
            }
            else if constexpr(std::is_same_v<T, float>)
            {
                return value.f32;
            }
            else
            {
                return value.f64;
            }
        }
    }

    class ExecutionDomain;

    class NEXUSWASM_API Instance final
    {
    public:
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&&) noexcept;
        Instance& operator=(Instance&&) noexcept;

        template<typename ResultType, typename... Args>
        [[nodiscard]]
        Result<ResultType> Call(
            const std::string_view name,
            Args... args
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

        ~Instance();

    private:
        explicit Instance(std::unique_ptr<detail::InstanceState> state) noexcept;

        [[nodiscard]]
        Result<detail::ScalerValue> CallScaler(
            std::string_view           name,
            const detail::ScalerValue* args,
            std::size_t                argumentCount,
            detail::ScalerKind         expectedResult
        );

        std::unique_ptr<detail::InstanceState> m_pState;

        friend class ExecutionDomain;
        friend class detail::ModuleGraph;
    };
}

#endif
