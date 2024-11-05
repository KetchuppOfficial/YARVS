#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include "common.hpp"
#include "elf_loader.hpp"
#include "reg_file.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory/memory.hpp"

namespace yarvs
{

class Hart final
{
public:

    Hart(const LoadableImage &image) : pc_{image.get_entry_point()}
    {
        for (auto &segment : image)
            mem_.store(segment.get_virtual_addr(), segment.begin(), segment.end());
    }

    void run()
    {
        for (;;)
        {
            auto raw_instr = mem_.load<RawInstruction>(pc_);
            Instruction instr = decoder_.decode(raw_instr);
            if (instr.id == InstrID::kEBREAK)
                break;
            executor_.execute(instr, *this);
        }
    }

    Word get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    RegFile &gprs() noexcept { return reg_file_; }
    const RegFile &gprs() const noexcept { return reg_file_; }

    Memory &memory() noexcept { return mem_; }
    const Memory &memory() const noexcept { return mem_; }

private:

    RegFile reg_file_;
    DoubleWord pc_;
    DoubleWord min_pc_ = 0, max_pc_ = 0;
    Memory mem_;
    [[no_unique_address]] Decoder decoder_;
    [[no_unique_address]] Executor executor_;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
