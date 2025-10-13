#pragma once

#include "qualifier.h"

namespace pyroc::math
{
template <typename T>
union vec<2, T>
{
    struct
    {
        T x;
        T y;
    };
    typename detail::storage<2, T>::type data;

    constexpr T& operator[](size_t i) { return data[i]; }
    constexpr const T& operator[](size_t i) const { return data[i]; }
};

}  // namespace pyroc::math
