#ifndef INCLUDE_MEMORY_PTE_HPP
#define INCLUDE_MEMORY_PTE_HPP

#include "common.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// page-table entry
class PTE
{
public:

    constexpr PTE() noexcept : entry_{0} {}
    constexpr PTE(DoubleWord entry) noexcept : entry_{entry} {}

    constexpr operator DoubleWord() noexcept { return entry_; }

    // indicates whether the PTE is valid
    constexpr bool get_V() const noexcept { return mask_bit<0>(entry_); }
    constexpr void set_V(bool v) noexcept { entry_ = set_bit<0>(entry_, v); }

    // indicates whether the page is readable
    constexpr bool get_R() const noexcept { return mask_bit<1>(entry_); }
    constexpr void set_R(bool r) noexcept { entry_ = set_bit<1>(entry_, r); }

    // indicates whether the page is writable
    constexpr bool get_W() const noexcept { return mask_bit<2>(entry_); }
    constexpr void set_W(bool w) noexcept { entry_ = set_bit<2>(entry_, w); }

    // indicates whether the page is executable
    constexpr bool get_E() const noexcept { return mask_bit<3>(entry_); }
    constexpr void set_E(bool e) noexcept { entry_ = set_bit<3>(entry_, e); }

    // indicates whether the page is accessible to user mode
    constexpr bool get_U() const noexcept { return mask_bit<4>(entry_); }
    constexpr void set_U(bool u) noexcept { entry_ = set_bit<4>(entry_, u); }

    // designates global mapping
    constexpr bool get_G() const noexcept { return mask_bit<5>(entry_); }
    constexpr void set_G(bool g) noexcept { entry_ = set_bit<5>(entry_, g); }

    // access bit
    constexpr bool get_A() const noexcept { return mask_bit<6>(entry_); }
    constexpr void set_A(bool a) noexcept { entry_ = set_bit<6>(entry_, a); }

    // dirty bit
    constexpr bool get_D() const noexcept { return mask_bit<7>(entry_); }
    constexpr void set_D(bool d) noexcept { entry_ = set_bit<7>(entry_, d); }

    // physical page number
    constexpr  DoubleWord get_ppn() const noexcept { return get_bits<53, 10>(entry_); }
    constexpr  void set_ppn(DoubleWord ppn) noexcept { entry_ = set_bits<53, 10>(entry_, ppn); }

    /*
     * The RSW field is reserved for use by the supervisor software; the implementation shall ignore
     * this field.
     */

    constexpr bool is_pointer_to_next_level_pte() const noexcept
    {
        return !mask_bits<3, 1>(entry_);
    }

    constexpr bool is_rwx_reserved() const noexcept
    {
        return mask_bits<2, 1>(entry_) == DoubleWord{0b100};
    }

    constexpr bool uses_reserved() const noexcept { return mask_bits<63, 54>(entry_); }

private:

    DoubleWord entry_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_PTE_HPP
