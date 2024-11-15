#ifndef INCLUDE_COMMON_HPP
#define INCLUDE_COMMON_HPP

#include <cstdint>
#include <concepts>
#include <climits>
#include <cstddef>

namespace yarvs
{

using Byte = std::uint8_t;
using HalfWord = std::uint16_t;
using Word = std::uint32_t;
using DoubleWord = std::uint64_t;

template<typename T>
concept riscv_type =
    std::same_as<T, Byte> || std::same_as<T, HalfWord> ||
    std::same_as<T, Word> || std::same_as<T, DoubleWord>;

using RawInstruction = std::uint32_t;

constexpr auto kXLen = sizeof(DoubleWord) * CHAR_BIT;
constexpr std::size_t kOpcodeBitLen = 7;

} // namespace yarvs

#endif // INCLUDE_COMMON_HPP
