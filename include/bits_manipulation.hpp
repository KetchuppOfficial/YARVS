#ifndef INCLUDE_BITS_MANIPULATION
#define INCLUDE_BITS_MANIPULATION

#include <bit>
#include <concepts>
#include <climits>

namespace yarvs
{

static_assert(std::endian::native == std::endian::little);

template<std::integral T>
constexpr std::size_t kNBits = sizeof(T) * CHAR_BIT;

/* returns bits [to; from] of the input, where to >= from
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 001011101
 *       ^               ^
 *       to             from
 */
template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T get_bits(T num)
{
    static_assert(to < kNBits<T>);
    static_assert(from <= to);

    num <<= kNBits<T> - to - 1;
    num >>= from;

    return num;
}

} // namespace yarvs

#endif // INCLUDE_BITS_MANIPULATION
