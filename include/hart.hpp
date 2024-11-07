#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include "common.hpp"
#include "reg_file.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory/memory.hpp"

namespace yarvs
{

class LoadableImage;

class Hart final
{
public:

    explicit Hart(DoubleWord pc) : pc_{pc} {}
    explicit Hart(const LoadableImage &image);

    void load_segments(const LoadableImage &image);
    void run();
    void clear();

    Word get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    RegFile &gprs() noexcept { return reg_file_; }
    const RegFile &gprs() const noexcept { return reg_file_; }

    Memory &memory() noexcept { return mem_; }
    const Memory &memory() const noexcept { return mem_; }

private:

    static constexpr DoubleWord kStackAddr = 0x7ffe49b14000;

    RegFile reg_file_;
    DoubleWord pc_;
    Memory mem_;
    [[no_unique_address]] Decoder decoder_;
    [[no_unique_address]] Executor executor_;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
