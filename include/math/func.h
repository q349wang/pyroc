#pragma once

#include "types/mat.h"

#include <cmath>

namespace pyroc::math
{
template <typename T>
constexpr mat<4, 4, T> perspective(T fovYRad, T aspect, T nearPlane, T farPlane)
{
    mat<4, 4, T> result = {};
    const T f = static_cast<T>(1) / static_cast<T>(tan(fovYRad / static_cast<T>(2)));

    result[0][0] = f / aspect;

    result[1][1] = f;

    result[2][2] = farPlane / (nearPlane - farPlane);
    result[3][2] = -farPlane / (nearPlane - farPlane);

    result[2][3] = static_cast<T>(1);

    return result;
}

template <typename T>
constexpr T radians(T degrees)
{
    return degrees * static_cast<T>(M_PI) / static_cast<T>(180);
}
}  // namespace pyroc::math
