#ifndef INCLUDE_PRIVILEGED_SUPERVISOR_SATP_HPP
#define INCLUDE_PRIVILEGED_SUPERVISOR_SATP_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// supervisor address translation and protection register
class SATP final
{
public:
    /*
     * Implementations are not required to support all MODE settings, and if satp is written with an
     * unsupported MODE, the entire write has no effect; no fields in satp are modified.
     */
    enum Mode : Byte
    {
        kBare = 0,  // no translation or protection
        /* 1-7 */   // reserved for standard use
        kSv39 = 8,  // page-based 39-bit virtual addressing
        kSv48 = 9,  // page-based 48-bit virtual addressing
        kSv57 = 10, // page-based 57-bit virtual addressing (not implemented in yarvs)
        kSv64 = 11, // reserved for page-based 64-bit virtual addressing
        /* 12-13 */ // reserved for standard use
        /* 14-15 */ // designated for custom use
    };

    constexpr SATP() noexcept : value_{0} {}
    constexpr SATP(DoubleWord value) noexcept : value_{value} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    // mode - current address translation scheme
    constexpr Mode get_mode() const noexcept { return Mode{get_bits_r<63, 60, Byte>(value_)}; }
    constexpr void set_mode(Mode m) noexcept
    {
        /*
         * Implementations are not required to support all MODE settings, and if satp is written
         * with an unsupported MODE, the entire write has no effect; no fields in satp are modified.
         */
        if (m == kBare || m == kSv39 || m == kSv48 || m == kSv57)
            value_ = set_bits<63, 60>(value_, Byte{m});
    }

    // asid - address space identifier
    constexpr HalfWord get_asid() const noexcept { return get_bits_r<59, 44, HalfWord>(value_); }
    constexpr void set_asid(HalfWord asid) noexcept { value_ = set_bits<59, 44>(value_, asid); }

    // ppn - physical page number of the root page table
    constexpr DoubleWord get_ppn() const noexcept { return mask_bits<43, 0>(value_); }
    constexpr void set_ppn(DoubleWord ppn) noexcept { value_ = set_bits<43, 0>(value_, ppn); }

    /*
     * 1) To select MODE=Bare, software must write zero to the remaining fields of satp. Attempting
     * to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect
     * on the value that the remaining fields assume and an UNSPECIFIED effect on address
     * translation and protection behavior.
     * 2) When SXLEN=64, all satp encodings corresponding to MODE=Bare are reserved for future
     * standard use.
     */
    constexpr void make_bare() noexcept { value_ = 0; }

private:
    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_SUPERVISOR_SATP_HPP
