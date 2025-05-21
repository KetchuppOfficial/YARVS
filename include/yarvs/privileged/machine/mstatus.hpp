#ifndef INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP
#define INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP

#include "yarvs/common.hpp"
#include "yarvs/bits_manipulation.hpp"

namespace yarvs
{

/* Machine status register
 *
 * | 63 |62  38| 37  | 36  |35      34|33      32|31  23| 22  | 21 | 20  | 19  | 18  |  17  |
 * | SD | WPRI | MBE | SBE | SXL[1:0] | UXL[1:0] | WPRI | TSR | TW | TWM | MXR | SUM | MPRV |
 *
 * |16     15|14     13|12      11|10      9|  8  |  7   |  6  |  5   |  4   |  3  |  2   |
 * | XS[1:0] | FS[1:0] | MPP[1:0] | VS[1:0] | SPP | MPIE | UBE | SPIE | WPRI | MIE | WPRI |
 *
 * |  1  |  0   |
 * | SIE | WPRI |
 */
class MStatus final
{
public:

    constexpr MStatus() noexcept : value_{0} {}
    constexpr MStatus(DoubleWord sstatus) noexcept : value_{sstatus} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    // enables or disables all interrupts in supervisor mode
    constexpr bool get_sie() const noexcept { return mask_bit<1>(value_); }
    constexpr void set_sie(bool sie) noexcept { value_ = set_bit<1>(value_, sie); }

    // enables or disables all interrupts in machine mode
    constexpr bool get_mie() const noexcept { return mask_bit<3>(value_); }
    constexpr void set_mie(bool sie) noexcept { value_ = set_bit<3>(value_, sie); }

    // indicates whether supervisor interrupts were enables prior to trapping into supervisor mode
    constexpr bool get_spie() const noexcept { return mask_bit<5>(value_); }
    constexpr void set_spie(bool spie) noexcept { value_ = set_bit<5>(value_, spie); }

    // TODO: add comment
    constexpr bool get_ube() const noexcept { return mask_bit<6>(value_); }
    constexpr void set_ube(bool ube) noexcept { value_ = set_bit<6>(value_, ube); }

    // indicates whether machine interrupts were enables prior to trapping into machine mode
    constexpr bool get_mpie() const noexcept { return mask_bit<7>(value_); }
    constexpr void set_mpie(bool spie) noexcept { value_ = set_bit<7>(value_, spie); }

    // indicates the privilege level at which a hart was executing before entering supervisor mode
    constexpr bool get_spp() const noexcept { return mask_bit<8>(value_); }
    constexpr void set_spp(bool spp) noexcept { value_ = set_bit<8>(value_, spp); }

    // TODO: add comment
    constexpr Byte get_vs() const noexcept { return get_bits_r<10, 9, Byte>(value_); }
    constexpr void set_vs(Byte vs) noexcept { value_ = set_bits<10, 9>(value_, vs); }

    // indicates the privilege level at which a hart was executing before entering machine mode
    constexpr Byte get_mpp() const noexcept { return get_bits_r<12, 11, Byte>(value_); }
    constexpr void set_mpp(Byte spp) noexcept { value_ = set_bits<12, 11>(value_, spp); }

    // TODO: add comment
    constexpr Byte get_xs() const noexcept { return get_bits_r<14, 13, Byte>(value_); }
    constexpr void set_xs(Byte xs) noexcept { value_ = set_bits<14, 13>(value_, xs); }

    // TODO: add coment
    constexpr Byte get_fs() const noexcept { return get_bits_r<16, 15, Byte>(value_); }
    constexpr void set_fs(Byte fs) noexcept { value_ = set_bits<16, 15>(value_, fs); }

    // modify privilege
    constexpr bool get_mprv() const noexcept { return mask_bit<17>(value_); }
    constexpr void set_mprv(bool mxr) noexcept { value_ = set_bit<17>(value_, mxr); }

    // permit supervisor user memory access
    constexpr bool get_sum() const noexcept { return mask_bit<18>(value_); }
    constexpr void set_sum(bool mxr) noexcept { value_ = set_bit<18>(value_, mxr); }

    // make executable readable
    constexpr bool get_mxr() const noexcept { return mask_bit<19>(value_); }
    constexpr void set_mxr(bool mxr) noexcept { value_ = set_bit<19>(value_, mxr); }

    // TODO: add comment
    constexpr bool get_twm() const noexcept { return mask_bit<20>(value_); }
    constexpr void set_twm(bool twm) noexcept { value_ = set_bit<20>(value_, twm); }

    // TODO: add comment
    constexpr bool get_tw() const noexcept { return mask_bit<21>(value_); }
    constexpr void set_tw(bool tw) noexcept { value_ = set_bit<21>(value_, tw); }

    // TODO: add comment
    constexpr bool get_tsr() const noexcept { return mask_bit<22>(value_); }
    constexpr void set_tsr(bool tsr) noexcept { value_ = set_bit<22>(value_, tsr); }

    // controls the value of XLEN for U-mode (UXLEN)
    constexpr Byte get_uxl() const noexcept { return get_bits<33, 32>(value_); }
    constexpr void set_uxl(Byte uxl) noexcept { value_ = set_bits<33, 32>(value_, uxl); }

    // controls the value of XLEN for S-mode (SXLEN)
    constexpr Byte get_sxl() const noexcept { return get_bits<35, 34>(value_); }
    constexpr void set_sxl(Byte sxl) noexcept { value_ = set_bits<35, 34>(value_, sxl); }

    // TODO: add comment
    constexpr bool get_sbe() const noexcept { return mask_bit<36>(value_); }
    constexpr void set_sbe(bool sbe) noexcept { value_ = set_bit<36>(value_, sbe); }

    // TODO: add comment
    constexpr bool get_mbe() const noexcept { return mask_bit<37>(value_); }
    constexpr void set_mbe(bool mbe) noexcept { value_ = set_bit<37>(value_, mbe); }

    // TODO: add comment
    constexpr bool get_sd() const noexcept { return mask_bit<63>(value_); }
    constexpr void set_sd(bool sd) noexcept { value_ = set_bit<63>(value_, sd); }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_MACHINE_MSTATUS_HPP
