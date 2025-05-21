#ifndef INCLUDE_PRIVILEGED_SUPERVISOR_SSTATUS_HPP
#define INCLUDE_PRIVILEGED_SUPERVISOR_SSTATUS_HPP

#include "yarvs/common.hpp"

#include "yarvs/privileged/machine/mstatus.hpp"

namespace yarvs
{

/* Supervisor status register
 *
 * | 63 |62  38|33      32|31  20| 19  | 18  |  17  |16     15|14     13|12  11|10      9|  8  |
 * | SD | WPRI | UXL[1:0] | WPRI | MXR | SUM | WPRI | XS[1:0] | FS[1:0] | WPRI | VS[1:0] | SPP |
 *
 * |  7   |  6  |  5   |4    2|  1  |  0   |
 * | WPRI | UBE | SPIE | WPRI | SIE | WPRI |
 */
class SStatus final
{
public:

    static constexpr DoubleWord kMask =
        0b1'0000000000000000000000000'11'000000000000'11111111111111'000'1'0;

    constexpr SStatus() = default;
    constexpr SStatus(DoubleWord sstatus) noexcept : mstatus_{sstatus} {}

    operator DoubleWord() noexcept { return mstatus_; }

    // enables or disables all interrupts in supervisor mode
    constexpr bool get_sie() const noexcept { return mstatus_.get_sie(); }
    constexpr void set_sie(bool sie) noexcept { mstatus_.set_sie(sie); }

    // indicates whether supervisor interrupts were enables prior to trapping into supervisor mode
    constexpr bool get_spie() const noexcept { return mstatus_.get_spie(); }
    constexpr void set_spie(bool spie) noexcept { mstatus_.set_spie(spie); }

    // TODO: add comment
    constexpr bool get_ube() const noexcept { return mstatus_.get_ube(); }
    constexpr void set_ube(bool ube) noexcept { mstatus_.set_ube(ube); }

    // indicates the privilege level at which a hart was executing before entering supervisor mode
    constexpr bool get_spp() const noexcept { return mstatus_.get_spp(); }
    constexpr void set_spp(bool spp) noexcept { mstatus_.set_spp(spp); }

    // TODO: add comment
    constexpr Byte get_vs() const noexcept { return mstatus_.get_vs(); }
    constexpr void set_vs(Byte vs) noexcept { mstatus_.set_vs(vs); }

    // TODO: add comment
    constexpr Byte get_fs() const noexcept { return mstatus_.get_fs(); }
    constexpr void set_fs(Byte fs) noexcept { mstatus_.set_fs(fs); }

    // TODO: add comment
    constexpr Byte get_xs() const noexcept { return mstatus_.get_xs(); }
    constexpr void set_xs(Byte xs) noexcept { mstatus_.set_xs(xs); }

    // permit supervisor user memory access
    constexpr bool get_sum() const noexcept { return mstatus_.get_sum(); }
    constexpr void set_sum(bool sum) noexcept { mstatus_.set_sum(sum); }

    // make executable readable
    constexpr bool get_mxr() const noexcept { return mstatus_.get_mxr(); }
    constexpr void set_mxr(bool mxr) noexcept { mstatus_.set_mxr(mxr); }

    // controls the value of XLEN for U-mode (UXLEN)
    constexpr Byte get_uxl() const noexcept { return mstatus_.get_uxl(); }
    constexpr void set_uxl(Byte uxl) noexcept { mstatus_.set_uxl(uxl); }

    // TODO: add comment
    constexpr bool get_sd() const noexcept { return mstatus_.get_sd(); }
    constexpr void set_sd(bool sd) noexcept { mstatus_.set_sd(sd); }

private:

    MStatus mstatus_;
};

} // namespace yarvs

#endif // INCLUDE_SUPERVISOR_SSTATUS_HPP
