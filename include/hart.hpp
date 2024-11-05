#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include "common.hpp"
#include "reg_file.hpp"
#include "memory/memory.hpp"

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

    template<typename Self>
    auto &&memory(this Self &&self) noexcept { return self.mem_; }

private:

    RegFile reg_file_;
    DoubleWord pc_;
    Memory mem_;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
