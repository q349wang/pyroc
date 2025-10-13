#pragma once

#include <cstdint>

namespace pyroc::math
{
template <size_t L, typename T>
union vec;

template <size_t N, size_t M, typename T>
union mat;

template <size_t L, typename T>
union qua;

namespace detail
{
template <size_t L, typename T>
struct storage
{
    typedef struct type
    {
        T data[L];

        constexpr T& operator[](size_t i) { return data[i]; }
        constexpr const T& operator[](size_t i) const { return data[i]; }
    } type;
};
}  // namespace detail

}  // namespace pyroc::math
