#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include "common.hpp"
#include "reg_file.hpp"

namespace yarvs
{

class Hart final
{
public:

    Hart(DoubleWord entry_point) noexcept : pc_{entry_point} {}

    Word get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    template<typename Self>
    auto &&gprs(this Self &&self) noexcept { return self.reg_file_; }

private:

    RegFile reg_file_;
    DoubleWord pc_;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
