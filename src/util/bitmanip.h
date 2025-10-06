#pragma once

#include <bitset>
#include <cstdint>

namespace pyroc::util
{
template <typename T>
constexpr size_t countBits(T value)
{
    const std::bitset<sizeof(T) * 8> bits(value);

    return bits.count();
}
}  // namespace pyroc::util