#pragma once

#include "qualifier.h"

namespace pyroc::math
{
template <typename T>
union vec<4, T>
{
    struct
    {
        T x;
        T y;
        T z;
        T w;
    };
    struct
    {
        T r;
        T g;
        T b;
        T a;
    };
    typename detail::storage<4, T>::type data;

    T& operator[](size_t i) { return data.data[i]; }
    const T& operator[](size_t i) const { return data.data[i]; }
};
}  // namespace pyroc::math
