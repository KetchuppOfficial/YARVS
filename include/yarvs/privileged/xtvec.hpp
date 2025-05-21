#ifndef INCLUDE_PRIVILEGED_XTVEC_HPP
#define INCLUDE_PRIVILEGED_XTVEC_HPP

#include "yarvs/common.hpp"
#include "yarvs/bits_manipulation.hpp"

namespace yarvs
{

// machine/supervisor trap vector base address register
class XTVec final
{
public:

    enum Mode : Byte
    {
        kDirect = 0,   // all exceptions set PC to BASE
        kVectored = 1, // asynchronous interrupts set PC to BASE + 4 * cause
        // >= 2: reserved
    };

    constexpr XTVec() noexcept : value_{0} {}
    constexpr XTVec(DoubleWord value) noexcept : value_{value} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    constexpr Mode get_mode() const noexcept { return static_cast<Mode>(mask_bits<1, 0>(value_)); }
    constexpr void set_mode(Byte mode) noexcept { value_ = set_bits<1, 0>(value_, mode); }

    constexpr DoubleWord get_base() const noexcept { return mask_bits<63, 2>(value_); }
    constexpr void set_base(DoubleWord base) noexcept { value_ = mask_bits<63, 2>(base); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_XTVEC_HPP
