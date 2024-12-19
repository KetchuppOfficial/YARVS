#ifndef INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP
#define INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

class MStatus final
{
public:

    constexpr MStatus() noexcept : value_{0} {}
    constexpr MStatus(DoubleWord sstatus) noexcept : value_{sstatus} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    // enables or disables all interrupts in supervisor mode
    constexpr bool get_mie() const noexcept { return mask_bit<3>(value_); }
    constexpr void set_mie(bool sie) noexcept { value_ = set_bit<3>(value_, sie); }

    // indicates whether supervisor interrupts were enables prior to trapping into supervisor mode
    constexpr bool get_mpie() const noexcept { return mask_bit<7>(value_); }
    constexpr void set_mpie(bool spie) noexcept { value_ = set_bit<7>(value_, spie); }

    // indicates the privilege level at which a hart was executing before entering supervisor mode
    constexpr Byte get_mpp() const noexcept { return get_bits_r<12, 11, Byte>(value_); }
    constexpr void set_mpp(Byte spp) noexcept { value_ = set_bits<12, 11>(value_, spp); }

    // modify privilege
    constexpr bool get_mprv() const noexcept { return mask_bit<17>(value_); }
    constexpr void set_mprv(bool mxr) noexcept { value_ = set_bit<17>(value_, mxr); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP
