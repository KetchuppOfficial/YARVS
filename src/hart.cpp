#include "hart.hpp"
#include "elf_loader.hpp"

namespace yarvs
{

Hart::Hart(const LoadableImage &image) : pc_{image.get_entry_point()} { load_segments(image); }

void Hart::load_segments(const LoadableImage &image)
{
    for (auto &segment : image)
        mem_.store(segment.get_virtual_addr(), segment.begin(), segment.end());
}

void Hart::run()
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

void Hart::clear()
{
    reg_file_.clear();
    pc_ = 0;
    mem_ = Memory{};
}

} // namespace yarvs
