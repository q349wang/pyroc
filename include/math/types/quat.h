#pragma once

#include "qualifier.h"

#include "vec.h"

namespace pyroc::math
{
template <typename L>
union quat
{
    vec<4, T> data;

    constexpr T& operator[](size_t i) { return data[i]; }
    constexpr const T& operator[](size_t i) const { return data[i]; }
};

template <typename T>
constexpr quat<T> operator*(const quat<T>& lhs, const quat<T>& rhs)
{
    quat<T> result = {};
    // cross(q, r) + q * r_w + q_w * r
    result[0] = (lhs[1] * rhs[2] - lhs[2] * rhs[1]) + lhs[0] * rhs[3] + lhs[3] * rhs[0];
    result[1] = (lhs[2] * rhs[0] - lhs[0] * rhs[2]) + lhs[1] * rhs[3] + lhs[3] * rhs[1];
    result[2] = (lhs[0] * rhs[1] - lhs[1] * rhs[0]) + lhs[2] * rhs[3] + lhs[3] * rhs[2];

    // q_w * r_w - dot(q, r)
    result[3] = lhs[3] * rhs[3] - (lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]);
    return result;
}

}  // namespace pyroc::math
