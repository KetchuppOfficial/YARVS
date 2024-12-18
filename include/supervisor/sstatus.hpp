#ifndef INCLUDE_SUPERVISOR_SSTATUS_HPP
#define INCLUDE_SUPERVISOR_SSTATUS_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// supervisor status register
class SStatus final
{
public:

    SStatus() noexcept : value_{0} {}
    SStatus(DoubleWord sstatus) noexcept : value_{sstatus} {}

    // enables or disables all interrupts in supervisor mode
    bool get_sie() const noexcept { return mask_bit<1>(value_); }

    // indicates whether supervisor interrupts were enables prior to trapping into supervisor mode
    bool get_spie() const noexcept { return mask_bit<5>(value_); }

    // indicates the privilege level at which a hart was executing before entering supervisor mode
    bool get_spp() const noexcept { return mask_bit<8>(value_); }
    void set_spp(bool spp) noexcept { value_ = set_bit<8>(value_, spp); }

    //
    Byte get_uxl() const noexcept { return get_bits<33, 32>(value_); }

    // make executable readable
    bool get_mxr() const noexcept { return mask_bit<19>(value_); }
    void set_mxr(bool mxr) noexcept { value_ = set_bit<19>(value_, mxr); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_SUPERVISOR_SSTATUS_HPP
