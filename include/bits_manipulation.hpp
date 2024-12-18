#ifndef INCLUDE_BITS_MANIPULATION
#define INCLUDE_BITS_MANIPULATION

#include <bit>
#include <climits>
#include <cstddef>
#include <concepts>
#include <type_traits>

namespace yarvs
{

static_assert(std::endian::native == std::endian::little);

template<std::integral T>
constexpr std::size_t kNBits = sizeof(T) * CHAR_BIT;

template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T get_mask() noexcept
{
    static_assert(from < to);
    static_assert(to < kNBits<T>);

    if constexpr (to == kNBits<T> - 1)
        return ~T{0} - ((T{1} << from) - 1);
    else
        return (T{1} << (to + 1)) - (T{1} << from);
}

/*
 * Masks bits [to; from] (from <= to) of the input
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000101110100000
 *       ^               ^
 *       to             from
 */
template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T mask_bits(T num) noexcept
{
    static_assert(from < to);
    static_assert(to < kNBits<T>);

    return num & get_mask<to, from, T>();
}

/*
 * Returns bits [to; from] (from <= to) of the input shifted to the low bits
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000000001011101
 *       ^               ^
 *       to             from
 */
template<std::size_t to, std::size_t from, std::unsigned_integral T>
constexpr T get_bits(T num) noexcept
{
    static_assert(from < to);
    static_assert(to < kNBits<T>);

    return mask_bits<to, from>(num) >> from;
}

template<std::size_t n, std::unsigned_integral T>
constexpr T set_bit(T num, bool bit) noexcept
{
    static_assert(n < kNBits<T>);

    return num | (static_cast<T>(bit) << n);
}

template<std::size_t to, std::size_t from, std::unsigned_integral T, std::unsigned_integral U>
constexpr T set_bits(T num, U value) noexcept
{
    static_assert(from < to);
    static_assert(to < kNBits<T>);

    auto mask = get_mask<to, from, T>();
    return (num & ~mask) | ((static_cast<T>(value) << from) & mask);
}

/*
 * The same as get_bits() but stores the result in a type possible different from the type of the
 * input, but long enough to store the bits in range [to; from]
 */
template<std::size_t to, std::size_t from, std::unsigned_integral R, std::unsigned_integral T>
constexpr R get_bits_r(T num) noexcept
{
    static_assert(from < to);
    static_assert(to < kNBits<T>);
    static_assert(to - from < kNBits<R>);

    return static_cast<R>(get_bits<to, from>(num));
}

/*
 * Masks a single bit of the input
 *
 *  15                             0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000000000100000
 *                       ^
 *                       n
 */
template<std::size_t n, std::unsigned_integral T>
constexpr T mask_bit(T num) noexcept
{
    static_assert(n < kNBits<T>);

    return num & (T{1} << n);
}

template<std::unsigned_integral T>
constexpr auto to_signed(T num) noexcept { return static_cast<std::make_signed_t<T>>(num); }

template<std::signed_integral T>
constexpr auto to_unsigned(T num) noexcept { return static_cast<std::make_unsigned_t<T>>(num); }

/*
 * Sign extened first <from_bits> of input to kNBits<To>
 *
 *  15  13               5         0
 *   0 1 0 0 1 0 1 1 1 0 1 0 0 1 1 0 -----> 0000101110100000
 *       ^               ^
 *       to             from
 */
template<std::size_t from_bits, std::unsigned_integral To, std::unsigned_integral From>
constexpr To sext(From num) noexcept
{
    static_assert(from_bits > 0);
    static_assert(from_bits <= kNBits<From>);
    static_assert(sizeof(From) <= sizeof(To));

    if constexpr (from_bits == kNBits<To>)
        return num;
    else if constexpr (from_bits == 8 || from_bits == 16 || from_bits == 32)
        return static_cast<To>(to_signed(num));
    else
    {
        auto sign_bit_mask = To{1} << from_bits - 1;
        return (num ^ sign_bit_mask) - sign_bit_mask;
    }
}

} // namespace yarvs

#endif // INCLUDE_BITS_MANIPULATION
