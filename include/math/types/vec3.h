#pragma once

#include "qualifier.h"

namespace pyroc::math
{
template <typename T>
union vec<3, T>
{
    struct
    {
        T x;
        T y;
        T z;
    };
    typename detail::storage<3, T>::type data;

    constexpr T& operator[](size_t i) { return data.data[i]; }
    constexpr const T& operator[](size_t i) const { return data.data[i]; }
};

template <typename T>
constexpr vec<3, T> cross(const vec<3, T>& lhs, const vec<3, T>& rhs)
{
    return vec<3, T>{
        .x = lhs.y * rhs.z - lhs.z * rhs.y,
        .y = lhs.z * rhs.x - lhs.x * rhs.z,
        .z = lhs.x * rhs.y - lhs.y * rhs.x,
    };
}
}  // namespace pyroc::math
