#ifndef INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP
#define INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP

#include <utility>

#include "common.hpp"
#include "bits_manipulation.hpp"

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
        switch (i)
        {
            case 0:
                return get_bits<20, 12>(addr_);
            case 1:
                return get_bits<29, 21>(addr_);
            case 2:
                return get_bits<38, 30>(addr_);
            case 3:
                return get_bits<47, 39>(addr_);
            case 4:
                return get_bits<56, 48>(addr_);
            default:
                std::unreachable();
        }
    }

private:

    DoubleWord addr_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_VIRTUAL_ADDRESS_HPP
