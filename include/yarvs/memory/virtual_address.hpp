#ifndef INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP
#define INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP

#include "yarvs/bits_manipulation.hpp"
#include "yarvs/common.hpp"

namespace yarvs
{

class VirtualAddress final
{
public:

    constexpr VirtualAddress(DoubleWord va) noexcept : addr_{va} {}

    constexpr operator DoubleWord() noexcept { return addr_; }

    constexpr DoubleWord get_page_offset() const noexcept { return mask_bits<11, 0>(addr_); }

    constexpr DoubleWord get_vpn(Byte i) const noexcept
    {
        assert(i <= 4);
        const auto shift = 9 * i;
        return get_bits(addr_, 20 + shift, 12 + shift);
    }

private:

    DoubleWord addr_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP
