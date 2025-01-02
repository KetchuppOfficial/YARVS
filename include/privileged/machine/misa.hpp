#ifndef INCLUDE_PRIVILEGED_MACHINE_MISA_HPP
#define INCLUDE_PRIVILEGED_MACHINE_MISA_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

class MISA final
{
public:

    enum Extensions : HalfWord
    {
        kA, // atomic
        kB, // B extension
        kC, // compressed extension
        kD, // double-precision floating-point extension
        kE, // RV32E/64E base ISA
        kF, // single-precision floating-point extension
        kG, // reserved
        kH, // hypervisor extension
        kI, // RV32I/64I/128I base ISA
        kJ, // reserved
        kK, // reserved
        kL, // reserved
        kM, // integer multiply/divide extension
        kN, // tentatively reserved for user-level interrupts extension
        kO, // reserved
        kP, // tentatively reserved for packed-simd extension
        kQ, // quad-precision floating-point extension
        kR, // reserved
        kS, // supervisor mode implemented
        kT, // reserved
        kU, // user mode implemented
        kV, // vector extension
        kW, // reserved
        kX, // non-standard extensions present
        kY, // reserved
        kZ  // reserved
    };

    enum XLen : Byte
    {
        k32 = 1,
        k64 = 2,
        k128 = 3
    };

    constexpr MISA() noexcept : value_{0} {}
    constexpr MISA(DoubleWord misa) noexcept : value_{misa} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    constexpr XLen get_xlen() const noexcept { return static_cast<XLen>(get_bits<63, 62>(value_)); }
    constexpr void set_xlen(XLen xlen) noexcept
    {
        value_ = set_bits<63, 62>(value_, static_cast<Byte>(xlen));
    }

    constexpr Extensions get_ext() const noexcept
    {
        return static_cast<Extensions>(get_bits<25, 0>(value_));
    }
    constexpr void set_ext(Extensions ext) noexcept
    {
        value_ = set_bits<25, 0>(value_, static_cast<HalfWord>(ext));
    }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_MACHINE_MISA_HPP
