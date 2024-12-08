#include <cstdint>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <format>

#include <elfio/elfio.hpp>

#include "hart.hpp"

namespace yarvs
{

Hart::Hart() : instr_cache_(kDefaultCacheCapacity, decoder_closure{}) {}

Hart::Hart(const std::filesystem::path &path) : Hart{}
{
    load_elf(path);

    #if 0 // temporary workaround caused by absence of address translation
    reg_file_.set_reg(2, kStackAddr); // stack pointer
    #else
    reg_file_.set_reg(2, 0x1000); // stack pointer
    #endif
}

void Hart::load_elf(const std::filesystem::path &path)
{
    ELFIO::elfio elf;
    if (!elf.load(path.native()))
        throw std::runtime_error{std::format("could not load ELF file \"{}\"", path.native())};

    if (auto error_msg = elf.validate(); !error_msg.empty())
        throw std::invalid_argument{std::format("ELF file is invalid: {}", error_msg)};

    if (elf.get_class() != ELFIO::ELFCLASS64)
        throw std::invalid_argument{"only 64-bit ELF files are supported"};

    if (auto type = elf.get_type(); type != ELFIO::ET_EXEC)
    {
        const char *type_str = [type]
        {
            if (ELFIO::ET_LOOS <= type && type <= ELFIO::ET_HIOS)
                return "os specific";

            if (ELFIO::ET_LOPROC <= type && type <= ELFIO::ET_HIPROC)
                return "processor specific";

            switch (type)
            {
                case ELFIO::ET_NONE:
                    return "unknown";
                case ELFIO::ET_REL:
                    return "relocatable file";
                case ELFIO::ET_DYN:
                    return "shared object";
                case ELFIO::ET_CORE:
                    return "core file";
                default:
                    std::unreachable();
            }
        }();

        throw std::invalid_argument{std::format("ELF is of type \"{}\"; executable expected",
                                    type_str)};
    }

    if (elf.get_machine() != ELFIO::EM_RISCV)
        throw std::invalid_argument{"only RISC-V executables are supported"};

    auto is_loadable = [](const auto &seg){ return seg->get_type() == ELFIO::PT_LOAD; };

    for (const auto &seg : std::views::filter(elf.segments, is_loadable))
    {
        auto seg_start = reinterpret_cast<const Byte *>(seg->get_data());
        mem_.store(seg->get_virtual_address(), seg_start, seg_start + seg->get_file_size());

        [[maybe_unused]] ELFIO::Elf_Word flags = seg->get_flags(); // for future use
    }

    pc_ = elf.get_entry();
}

std::uintmax_t Hart::run()
{
    run_ = true;

    std::uintmax_t instr_count = 0;
    for (; run_; ++instr_count)
    {
        auto raw_instr = mem_.load<RawInstruction>(pc_);
        auto &instr = instr_cache_.lookup_update(raw_instr);
        Executor::execute(*this, instr);
    }

    return instr_count;
}

void Hart::clear()
{
    reg_file_.clear();
    pc_ = 0;
    mem_ = Memory{};

    instr_cache_.clear();

    run_ = false;
    status_ = 0;
}

} // namespace yarvs
