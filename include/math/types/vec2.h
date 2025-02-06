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

    T& operator[](size_t i) { return data.data[i]; }
    T const& operator[](size_t i) const { return data.data[i]; }
};

}  // namespace pyroc::math