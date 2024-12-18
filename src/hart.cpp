#include <filesystem>
#include <ranges>
#include <map>
#include <stdexcept>
#include <format>
#include <utility>
#include <cstdint>

#include <elfio/elfio.hpp>

#include "hart.hpp"
#include "common.hpp"
#include "decoder.hpp"
#include "executor.hpp"

#include "memory/memory.hpp"

#include "memory/virtual_address.hpp"

namespace yarvs
{

Hart::Hart(const Config &config) : mem_{csrs_}, config_{config}, bb_cache_{kDefaultCacheCapacity}
{
    csrs_.satp.set_mode(config_.get_translation_mode());
    csrs_.satp.set_ppn(kPPN);

    static_assert(sizeof(void *) == sizeof(DoubleWord));
    csrs_.stvec.set_base(reinterpret_cast<DoubleWord>(default_exception_handler));
}

Hart::Hart(const Config &config, const std::filesystem::path &path) : Hart{config}
{
    load_elf(path);
    reg_file_.set_reg(kSP, config_.get_stack_top());
}

namespace
{

template<std::ranges::input_range R>
auto collect_pages(R &&loadable_segments, DoubleWord stack_top, DoubleWord n_stack_pages)
{
    std::map<DoubleWord, ELFIO::Elf_Word> pages;
    for (const auto &seg : loadable_segments)
    {
        const auto va = seg->get_virtual_address();
        auto first_page_addr = mask_bits<63, Memory::kPageBits>(va);
        const auto last_page_addr = mask_bits<63, Memory::kPageBits>(va + seg->get_memory_size());

        auto flags = seg->get_flags();
        for (; first_page_addr <= last_page_addr; first_page_addr += Memory::kPageSize)
            pages.emplace(first_page_addr, flags);
    }

    const auto kStackLastPage = mask_bits<63, Memory::kPageBits>(stack_top);
    for (auto i = 0; i != n_stack_pages; ++i)
        pages.emplace(kStackLastPage - i * Memory::kPageSize, ELFIO::PF_R | ELFIO::PF_W);

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

    auto satp = csrs_.satp;
    csrs_.satp.set_mode(SATP::Mode::kBare);

    constexpr DoubleWord kStackPages = 4;
    const std::map<DoubleWord, ELFIO::Elf_Word> pages =
        collect_pages(loadable_segments, config_.get_stack_top(), config_.get_n_stack_pages());

    const Byte kLevels = satp.get_mode() - 5;
    DoubleWord table_end_ppn = 1 + satp.get_ppn();
    DoubleWord data_begin_ppn = kFreePhysMemBegin / Memory::kPageSize;

    std::map<DoubleWord, DoubleWord> va_to_pa;
    for (auto [page, rwx] : pages)
    {
        const VirtualAddress va = page;
        auto a = satp.get_ppn() * Memory::kPageSize;
        for (Byte i = kLevels - 1; i > 0; --i)
        {
            const auto pa = a + va.get_vpn(i) * sizeof(PTE);
            PTE pte = mem_.load<DoubleWord>(pa);
            if (pte.get_V())
                a = pte.get_ppn() * Memory::kPageSize;
            else
            {
                pte = 0b10001; // pointer to next level pte
                assert(pte.is_pointer_to_next_level_pte());
                pte.set_ppn(table_end_ppn);
                mem_.store(pa, static_cast<DoubleWord>(pte));
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
        mem_.store(pa, static_cast<DoubleWord>(pte));
    }

    for (const auto &seg : loadable_segments)
    {
        auto v_page = mask_bits<63, Memory::kPageBits>(seg->get_virtual_address());
        auto pa = va_to_pa.at(v_page) |
            mask_bits<Memory::kPageBits - 1, 0>(seg->get_virtual_address());
        auto seg_start = reinterpret_cast<const Byte *>(seg->get_data());
        mem_.store(pa, seg_start, seg_start + seg->get_file_size());
    }

    pc_ = elf.get_entry();

    csrs_.satp = satp;
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
