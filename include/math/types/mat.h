#pragma once

#include "qualifier.h"

namespace pyroc::math
{
template <size_t N, size_t M, typename T>
union mat
{
    typename detail::storage<N, vec<M, T>>::type cols;

    constexpr vec<M, T>& operator[](size_t i) { return cols[i]; }
    constexpr vec<M, T>& operator[](size_t i) const { return cols[i]; }

    static constexpr mat<N, M, T> identity()
    {
        mat<N, M, T> result = {};
        for (size_t i = 0; i < std::min(N, M); ++i)
        {
            result[i][i] = static_cast<T>(1);
        }
        return result;
    }
};

template <size_t L, size_t N, size_t M, typename T>
constexpr mat<L, M, T> operator*(const mat<L, N, T>& lhs, const mat<N, M, T>& rhs)
{
    mat<L, M, T> result = {};
    for (size_t i = 0; i < L; ++i)
    {
        for (size_t j = 0; j < M; ++j)
        {
            for (size_t k = 0; k < N; ++k)
            {
                result[i][j] += lhs[i][k] * rhs[k][j];
            }
        }
    }
    return result;
}

template <size_t N, size_t M, typename T>
constexpr vec<N, T> operator*(const mat<N, M, T>& lhs, const vec<N, T>& rhs)
{
    vec<N, T> result = {};
    for (size_t i = 0; i < N; ++i)
    {
        for (size_t j = 0; j < M; ++j)
        {
            result[i] += lhs[i][j] * rhs[j];
        }
    }
    return result;
}
}  // namespace pyroc::math
