#ifndef INCLUDE_REG_FILE_HPP
#define INCLUDE_REG_FILE_HPP

#include <cstddef>
#include <cassert>
#include <array>

#include "common.hpp"

namespace yarvs
{

class RegFile final
{
public:

    static constexpr std::size_t kNRegs = 32;

    using reg_type = DoubleWord;

    RegFile() = default;

    RegFile(const RegFile &rhs) = delete;
    RegFile &operator=(const RegFile &rhs) = delete;

    RegFile(RegFile &&rhs) = delete;
    RegFile &operator=(RegFile &&rhs) = delete;

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

private:

    std::array<reg_type, kNRegs> gprs_{};
};

} // namespace yarvs

#endif // INCLUDE_REG_FILE_HPP
