#include "hart.hpp"
#include "elf_loader.hpp"

namespace yarvs
{

Hart::Hart(const LoadableImage &image) : pc_{image.get_entry_point()}
{
    load_segments(image);
    #if 0 // temporary workaround caused by absence of address translation
    reg_file_.set_reg(2, kStackAddr); // stack pointer
    #else
    reg_file_.set_reg(2, 0x1000); // stack pointer
    #endif
}

void Hart::load_segments(const LoadableImage &image)
{
    for (auto &segment : image)
        mem_.store(segment.get_virtual_addr(), segment.begin(), segment.end());
}

void Hart::run()
{
    run_ = true;

    do
    {
        auto raw_instr = mem_.load<RawInstruction>(pc_);
        Instruction instr = decoder_.decode(raw_instr);
        executor_.execute(instr, *this);
    }
    while (run_);
}

void Hart::clear()
{
    reg_file_.clear();
    pc_ = 0;
    mem_ = Memory{};
}

} // namespace yarvs
