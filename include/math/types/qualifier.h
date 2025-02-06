#pragma once

#include <cstdint>

namespace pyroc::math
{
template <size_t L, typename T>
union vec;

template <size_t L, typename T>
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
    } type;
};
}  // namespace detail

}  // namespace pyroc::math