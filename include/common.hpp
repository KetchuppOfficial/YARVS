#ifndef INCLUDE_COMMON_HPP
#define INCLUDE_COMMON_HPP

#include <cstdint>
#include <concepts>
#include <climits>

namespace yarvs
{

using Byte = uint8_t;
using HalfWord = uint16_t;
using Word = uint32_t;
using DoubleWord = uint64_t;

template<typename T>
concept riscv_type =
    std::same_as<T, Byte> || std::same_as<T, HalfWord> ||
    std::same_as<T, Word> || std::same_as<T, DoubleWord>;

using RawInstruction = uint32_t;

constexpr auto kXLen = sizeof(DoubleWord) * CHAR_BIT;
constexpr std::size_t kOpcodeBitLen = 7;

} // namespace yarvs

#endif // INCLUDE_COMMON_HPP
