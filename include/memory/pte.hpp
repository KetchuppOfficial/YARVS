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

    PTE() noexcept : entry_{0} {}
    PTE(DoubleWord entry) noexcept : entry_{entry} {}

    operator DoubleWord() { return entry_; }

    // indicates whether the PTE is valid
    bool get_V() const noexcept { return mask_bit<0>(entry_); }

    // indicates whether the page is readable
    bool get_R() const noexcept { return mask_bit<1>(entry_); }

    // indicates whether the page is writable
    bool get_W() const noexcept { return mask_bit<2>(entry_); }

    // indicates whether the page is executable
    bool get_E() const noexcept { return mask_bit<3>(entry_); }

    // indicates whether the page is accessible to user mode
    bool get_U() const noexcept { return mask_bit<4>(entry_); }

    // designates global mapping
    bool get_G() const noexcept { return mask_bit<5>(entry_); }

    // access bit
    bool get_A() const noexcept { return mask_bit<6>(entry_); }
    void set_A() noexcept { entry_ |= (DoubleWord{1} << 6); }

    // dirty bit
    bool get_D() const noexcept { return mask_bit<7>(entry_); }
    void set_D() noexcept { entry_ |= (DoubleWord{1} << 7); }

    // physical page number
    DoubleWord get_ppn() const noexcept { return get_bits<53, 10>(entry_); }

    /*
     * The RSW field is reserved for use by the supervisor software; the implementation shall ignore
     * this field.
     */

    bool is_pointer_to_next_level_pte() const noexcept { return !mask_bits<3, 1>(entry_); }
    bool is_rwx_reserved() const noexcept { return mask_bits<2, 1>(entry_) == DoubleWord{0b100}; }

    bool uses_reserved() const noexcept { return mask_bits<63, 54>(entry_); }

private:

    DoubleWord entry_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_PTE_HPP
