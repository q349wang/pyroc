#pragma once

#include "qualifier.h"

#include <cmath>

namespace pyroc::math
{
template <size_t L, typename T>
constexpr vec<L, T> operator*(T lhs, const vec<L, T>& rhs)
{
    vec<L, T> result = {};
    for (size_t i = 0; i < L; ++i)
    {
        result[i] = lhs * rhs[i];
    }
    return result;
}

template <size_t L, typename T>
constexpr vec<L, T> operator+(const vec<L, T>& lhs, const vec<L, T>& rhs)
{
    vec<L, T> result = {};
    for (size_t i = 0; i < L; ++i)
    {
        result[i] = lhs[i] + rhs[i];
    }
    return result;
}

template <size_t L, typename T>
constexpr vec<L, T> operator-(const vec<L, T>& lhs, const vec<L, T>& rhs)
{
    vec<L, T> result = {};
    for (size_t i = 0; i < L; ++i)
    {
        result[i] = lhs[i] - rhs[i];
    }
    return result;
}

template <size_t L, typename T>
constexpr T dot(const vec<L, T>& lhs, const vec<L, T>& rhs)
{
    T result = static_cast<T>(0);
    for (size_t i = 0; i < L; ++i)
    {
        result += lhs[i] * rhs[i];
    }
    return result;
}

template <size_t L, typename T>
constexpr vec<L, T> normalize(const vec<L, T>& v)
{
    const T length2 = static_cast<T>(dot(v, v));
    if (length2 == static_cast<T>(1))
    {
        return v;
    }

    vec<L, T> result = {};
    if (length2 > static_cast<T>(0))
    {
        const T invLength = static_cast<T>(1) / static_cast<T>(sqrt(length2));
        result = invLength * v;
    }
    return result;
}
}  // namespace pyroc::math
