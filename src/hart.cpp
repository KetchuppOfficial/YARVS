#include <cstdint>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <format>
#include <map>
#include <ranges>

#include <elfio/elfio.hpp>

#include "hart.hpp"
#include "common.hpp"
#include "decoder.hpp"
#include "executor.hpp"

#include "memory/memory.hpp"

#include "memory/virtual_address.hpp"

namespace yarvs
{

Hart::Hart() : mem_{csrs_}, bb_cache_{kDefaultCacheCapacity}
{
    csrs_.satp.set_mode(SATP::Mode::kSv48);
    csrs_.satp.set_ppn(kPPN);

    static_assert(sizeof(void *) == sizeof(DoubleWord));
    csrs_.stvec.set_base(reinterpret_cast<DoubleWord>(default_exception_handler));
}

Hart::Hart(const std::filesystem::path &path) : Hart{}
{
    load_elf(path);
    reg_file_.set_reg(kSP, kStackAddr);
}

namespace
{

template<DoubleWord kStackAddr, DoubleWord kStackPages, std::ranges::input_range R>
auto collect_pages(R &&loadable_segments)
{
    constexpr auto kPageSize = DoubleWord{1} << 12;

    std::map<DoubleWord, ELFIO::Elf_Word> pages;
    for (const auto &seg : loadable_segments)
    {
        auto first_page_addr = mask_bits<63, 12>(seg->get_virtual_address());
        const auto last_page_addr =
            mask_bits<63, 12>(seg->get_virtual_address() + seg->get_memory_size());

        auto flags = seg->get_flags();
        for (; first_page_addr <= last_page_addr; first_page_addr += kPageSize)
            pages.emplace(first_page_addr, flags);
    }

    constexpr auto kStackLastPage = mask_bits<63, 12>(kStackAddr);
    for (auto i = 0; i != kStackPages; ++i)
        pages.emplace(kStackLastPage - i * kPageSize, ELFIO::PF_R | ELFIO::PF_W);

    return pages;
}

} // unnamed namespace

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
    auto loadable_segments = std::views::filter(elf.segments, is_loadable);

    constexpr DoubleWord kStackPages = 4;
    const std::map<DoubleWord, ELFIO::Elf_Word> pages =
        collect_pages<kStackAddr, kStackPages>(loadable_segments);

    const Byte kLevels = csrs_.satp.get_mode() - 5;
    DoubleWord table_end_ppn = 1 + csrs_.satp.get_ppn();
    DoubleWord data_begin_ppn = kFreePhysMemBegin / Memory::kPageSize;

    std::map<DoubleWord, DoubleWord> va_to_pa;
    for (auto [page, rwx] : pages)
    {
        const VirtualAddress va = page;
        auto a = csrs_.satp.get_ppn() * Memory::kPageSize;
        for (Byte i = kLevels - 1; i > 0; --i)
        {
            const auto pa = a + va.get_vpn(i) * sizeof(PTE);
            PTE pte = mem_.pm_load<DoubleWord>(pa);
            if (pte.get_V())
                a = pte.get_ppn() * Memory::kPageSize;
            else
            {
                pte = 0b10001; // pointer to next level pte
                assert(pte.is_pointer_to_next_level_pte());
                pte.set_ppn(table_end_ppn);
                mem_.pm_store(pa, static_cast<DoubleWord>(pte));
                a = table_end_ppn * Memory::kPageSize;
                ++table_end_ppn;
            }
        }

        va_to_pa.emplace(page, data_begin_ppn * Memory::kPageSize);

        PTE pte = 0b10001; // U V
        pte.set_R(rwx & ELFIO::PF_R);
        pte.set_W(rwx & ELFIO::PF_W);
        pte.set_E(rwx & ELFIO::PF_X);
        pte.set_ppn(data_begin_ppn++);

        const auto pa = a + va.get_vpn(0) * sizeof(PTE);
        mem_.pm_store(pa, static_cast<DoubleWord>(pte));
    }

    for (const auto &seg : loadable_segments)
    {
        auto seg_start = reinterpret_cast<const Byte *>(seg->get_data());
        auto v_page = mask_bits<63, 12>(seg->get_virtual_address());
        auto pa = va_to_pa.at(v_page) | mask_bits<11, 0>(seg->get_virtual_address());
        std::copy_n(seg->get_data(), seg->get_file_size(), &mem_.physical_mem_[pa]);
    }

    pc_ = elf.get_entry();
}

std::uintmax_t Hart::run()
{
    run_ = true;

    BasicBlock bb;
    std::uintmax_t instr_count = 0;
    while (run_)
    {
        if (auto bb_it = bb_cache_.lookup(pc_); bb_it != bb_cache_.end())
        {
            for (const auto &instr : bb_it->second)
            {
                Executor::execute(*this, instr);
                ++instr_count;
            }
        }
        else
        {
            bb.reserve(kDefaultBBLength);

            const auto bb_pc = pc_;

            for (bool is_terminator = false; !is_terminator; ++instr_count)
            {
                auto raw_instr = mem_.fetch(pc_);
                const auto &instr = bb.emplace_back(Decoder::decode(raw_instr));
                is_terminator = Executor::execute(*this, instr);
            }

            bb_cache_.update(bb_pc, std::move(bb));
            bb.clear(); // just in case: moved-from object is in valid but unspecified state
        }
    }

    return instr_count;
}

} // namespace yarvs
