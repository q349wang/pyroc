#pragma once

#include "qualifier.h"

namespace pyroc::math
{
template <size_t N, size_t M, typename T>
union mat
{
    typename detail::storage<N, vec<M, T>>::type rows;

    vec<M, T>& operator[](size_t i) { return rows[i]; }
    const vec<M, T>& operator[](size_t i) const { return rows[i]; }

    static mat<N, M, T> identity()
    {
        mat<N, M, T> result = {};
        for (size_t i = 0; i < N && i < M; ++i)
        {
            result.rows[i].data[i] = static_cast<T>(1);
        }
        return result;
    }
};

template <size_t L, size_t N, size_t M, typename T>
mat<L, M, T> operator*(const mat<L, N, T>& lhs, const mat<N, M, T>& rhs)
{
    mat<L, M, T> result = {};
    for (size_t i = 0; i < L; ++i)
    {
        for (size_t j = 0; j < M; ++j)
        {
            for (size_t k = 0; k < N; ++k)
            {
                result.rows[i].data[j] += lhs.rows[i].data[k] * rhs.rows[k].data[j];
            }
        }
    }
    return result;
}

}  // namespace pyroc::math
