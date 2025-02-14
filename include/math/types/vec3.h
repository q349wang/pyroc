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

    T& operator[](size_t i) { return data.data[i]; }
    const T& operator[](size_t i) const { return data.data[i]; }
};
}  // namespace pyroc::math