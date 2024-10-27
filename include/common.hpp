#ifndef INCLUDE_COMMON_HPP
#define INCLUDE_COMMON_HPP

#include <cstdint>
#include <climits>

namespace yarvs
{

using Byte = uint8_t;
using HalfWord = uint16_t;
using Word = uint32_t;
using DoubleWord = uint64_t;

using RawInstruction = uint32_t;

constexpr auto kXLen = sizeof(DoubleWord) * CHAR_BIT;
constexpr std::size_t kOpcodeBitLen = 7;

} // namespace yarvs

#endif // INCLUDE_COMMON_HPP
