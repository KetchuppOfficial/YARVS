#ifndef INCLUDE_SUPERVISOR_STVEC_HPP
#define INCLUDE_SUPERVISOR_STVEC_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// supervisor trap vector base address register
class STVec final
{
public:

    enum Mode : Byte
    {
        kDirect = 0,   // all exceptions set PC to BASE
        kVectored = 1, // asynchronous interrupts set PC to BASE + 4 * cause
        // >= 2: reserved
    };

    STVec() noexcept : value_{0} {}
    STVec(DoubleWord value) noexcept : value_{value} {}

    Mode get_mode() const noexcept { return static_cast<Mode>(mask_bits<1, 0>(value_)); }
    void set_mode(Byte mode) noexcept { value_ = set_bits<1, 0>(value_, mode); }

    DoubleWord get_base() const noexcept { return get_bits<63, 2>(value_); }
    void set_base(DoubleWord base) noexcept { value_ = set_bits<63, 2>(value_, base); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_SUPERVISOR_STVEC_HPP
