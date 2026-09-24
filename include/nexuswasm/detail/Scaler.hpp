#pragma once

#ifndef NEXUSWASM_DETAIL_SCALER_HEADER
#define NEXUSWASM_DETAIL_SCALER_HEADER

#include <cstdint>
#include <type_traits>

namespace nexus::detail
{
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

#endif
