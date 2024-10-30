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

/* returns bits [to; from] (from <= to) of the input shifted to the low bits
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000000001011101
 *       ^               ^
 *       to             from
 */
template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T get_bits(T num)
{
    static_assert(to < kNBits<T>);
    static_assert(from <= to);

    return (num << kNBits<T> - to - 1) >> from;
}

template<std::size_t n, std::unsigned_integral T>
constexpr T get_bit(T num)
{
    static_assert(n < kNBits<T>);

    return (num & (T{1} << n)) >> n;
}

/* masks bits [to; from] (from <= to) of the input
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000101110100000
 *       ^               ^
 *       to             from
 */
template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T mask_bits(T num)
{
    static_assert(to < kNBits<T>);
    static_assert(from <= to);

    auto mask = (T{1} << (to + 1)) - (T{1} << from);
    return num & mask;
}

template<std::size_t from_bits, std::unsigned_integral To, std::unsigned_integral From>
constexpr To sext(From num)
{
    static_assert(from_bits > 0);
    static_assert(from_bits <= kNBits<From>);
    static_assert(sizeof(From) < sizeof(To));

    auto sign_bit_mask = To{1} << from_bits - 1;
    return (num ^ sign_bit_mask) - sign_bit_mask;
}

} // namespace yarvs

#endif // INCLUDE_BITS_MANIPULATION
