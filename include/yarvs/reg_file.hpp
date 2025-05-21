#ifndef INCLUDE_REG_FILE_HPP
#define INCLUDE_REG_FILE_HPP

#include <array>
#include <cassert>
#include <cstddef>

#include "yarvs/common.hpp"

namespace yarvs
{

class RegFile final
{
public:

    static constexpr std::size_t kNRegs = 32;

    using reg_type = DoubleWord;

    RegFile() = default;

    reg_type get_reg(std::size_t i) const noexcept
    {
        assert(i < kXLen);
        return gprs_[i];
    }

    void set_reg(std::size_t i, reg_type new_value) noexcept
    {
        if (i != 0)
        {
            assert(i < 32);
            gprs_[i] = new_value;
        }
    }

    void clear() { gprs_.fill(0); }

    auto begin() noexcept { return gprs_.begin(); }
    auto begin() const noexcept { return gprs_.begin(); }
    auto cbegin() const noexcept { return begin(); }

    auto end() noexcept { return gprs_.end(); }
    auto end() const noexcept { return gprs_.end(); }
    auto cend() const noexcept { return end(); }

    auto rbegin() noexcept { return gprs_.rbegin(); }
    auto rbegin() const noexcept { return gprs_.rbegin(); }
    auto crbegin() const noexcept { return rbegin(); }

    auto rend() noexcept { return gprs_.rend(); }
    auto rend() const noexcept { return gprs_.rend(); }
    auto crend() const noexcept { return rend(); }

private:

    std::array<reg_type, kNRegs> gprs_{};
};

} // namespace yarvs

#endif // INCLUDE_REG_FILE_HPP
