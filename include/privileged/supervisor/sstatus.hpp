#ifndef INCLUDE_PRIVILEGED_SUPERVISOR_SSTATUS_HPP
#define INCLUDE_PRIVILEGED_SUPERVISOR_SSTATUS_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// supervisor status register
class SStatus final
{
public:

    constexpr SStatus() noexcept : value_{0} {}
    constexpr SStatus(DoubleWord sstatus) noexcept : value_{sstatus} {}

    operator DoubleWord() noexcept { return value_; }

    // enables or disables all interrupts in supervisor mode
    constexpr bool get_sie() const noexcept { return mask_bit<1>(value_); }
    constexpr void set_sie(bool sie) noexcept { value_ = set_bit<1>(value_, sie); }

    // indicates whether supervisor interrupts were enables prior to trapping into supervisor mode
    constexpr bool get_spie() const noexcept { return mask_bit<5>(value_); }
    constexpr void set_spie(bool spie) noexcept { value_ = set_bit<5>(value_, spie); }

    // indicates the privilege level at which a hart was executing before entering supervisor mode
    constexpr bool get_spp() const noexcept { return mask_bit<8>(value_); }
    constexpr void set_spp(bool spp) noexcept { value_ = set_bit<8>(value_, spp); }

    // controls the value of XLEN for U-mode (UXLEN)
    constexpr Byte get_uxl() const noexcept { return get_bits<33, 32>(value_); }
    constexpr void set_uxl(Byte uxl) noexcept { value_ = set_bits<33, 32>(value_, uxl); }

    // permit supervisor user memory access
    constexpr bool get_sum() const noexcept { return mask_bit<18>(value_); }
    constexpr void set_sum(bool mxr) noexcept { value_ = set_bit<18>(value_, mxr); }

    // make executable readable
    constexpr bool get_mxr() const noexcept { return mask_bit<19>(value_); }
    constexpr void set_mxr(bool mxr) noexcept { value_ = set_bit<19>(value_, mxr); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_SUPERVISOR_SSTATUS_HPP
