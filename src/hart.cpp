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

#include "memory/memory.hpp"
#include "memory/virtual_address.hpp"

#include "privileged/machine/misa.hpp"
#include "privileged/xtvec.hpp"
#include "privileged/supervisor/satp.hpp"
#include "privileged/supervisor/sstatus.hpp"

namespace yarvs
{

Hart::Hart() : mem_{csrs_, priv_level_}, bb_cache_{kDefaultCacheCapacity}
{
    csrs_.set_misa(MISA::Extensions::kI | MISA::Extensions::kS | MISA::Extensions::kU);
}

Hart::Hart(const Config &config, const std::filesystem::path &path) : Hart{}
{
    SStatus sstatus;
    sstatus.set_mxr(true);
    csrs_.set_sstatus(sstatus);

    SATP satp;
    satp.set_mode(config.get_translation_mode());
    satp.set_ppn(kPPN);
    csrs_.set_satp(satp);

    static_assert(sizeof(void *) == sizeof(DoubleWord));
    XTVec mtvec;
    mtvec.set_base(0);
    csrs_.set_mtvec(mtvec);

    load_elf(config, path);

    gprs_.set_reg(kSP, config.get_stack_top());
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

    const auto stack_last_page = mask_bits<63, Memory::kPageBits>(stack_top);
    for (auto i = 0; i != n_stack_pages; ++i)
        pages.emplace(stack_last_page - i * Memory::kPageSize, ELFIO::PF_R | ELFIO::PF_W);

    return pages;
}

} // unnamed namespace

void Hart::load_elf(const Config &config, const std::filesystem::path &path)
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
    std::map<DoubleWord, ELFIO::Elf_Word> pages =
        collect_pages(loadable_segments, config.get_stack_top(), config.get_n_stack_pages());

    // default exception handler
    const auto stack_last_page = mask_bits<63, Memory::kPageBits>(config.get_stack_top());
    const auto handler_va = stack_last_page - config.get_n_stack_pages() * Memory::kPageSize;

    const Byte kLevels = csrs_.get_satp().get_mode() - 5;
    DoubleWord table_end_ppn = 1 + csrs_.get_satp().get_ppn();
    DoubleWord data_begin_ppn = kFreePhysMemBegin / Memory::kPageSize;

    std::map<DoubleWord, DoubleWord> va_to_pa;
    for (auto [page, rwx] : pages)
    {
        const VirtualAddress va = page;
        auto a = csrs_.get_satp().get_ppn() * Memory::kPageSize;
        for (Byte i = kLevels - 1; i > 0; --i)
        {
            const auto pa = a + va.get_vpn(i) * sizeof(PTE);
            PTE pte = mem_.load<DoubleWord>(pa).value();
            if (pte.get_V())
                a = pte.get_ppn() * Memory::kPageSize;
            else
            {
                pte = 0b10001; // pointer to next level pte
                assert(pte.is_pointer_to_next_level_pte());
                pte.set_ppn(table_end_ppn);
                mem_.store(pa, +pte);
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
        mem_.store(pa, +pte);
    }

    for (const auto &seg : loadable_segments)
    {
        auto v_page = mask_bits<63, Memory::kPageBits>(seg->get_virtual_address());
        auto pa = va_to_pa.at(v_page) |
            mask_bits<Memory::kPageBits - 1, 0>(seg->get_virtual_address());
        auto seg_start = reinterpret_cast<const Byte *>(seg->get_data());
        mem_.store(pa, seg_start, seg_start + seg->get_file_size());
    }

    mem_.store(0, kDefaultExceptionHandler.begin(), kDefaultExceptionHandler.end());

    pc_ = elf.get_entry();
}

std::uintmax_t Hart::run()
{
    priv_level_ = PrivilegeLevel::kUser;
    run_ = true;

    BasicBlock bb;

    /*
     * Instruction that raises exception isn't considered executed until return from the exception
     * handler.
     */
    std::uintmax_t instr_count = 0;
    while (run_)
    {
        exception:

        if (auto bb_it = bb_cache_.lookup(pc_); bb_it != bb_cache_.end())
        {
            for (const auto &instr : bb_it->second)
            {
                if (!execute(instr)) [[unlikely]]
                    goto exception;
                ++instr_count;
            }
        }
        else
        {
            bb.reserve(kDefaultBBLength);

            const auto bb_pc = pc_;

            for (;;)
            {
                auto maybe_raw_instr = mem_.fetch(pc_);
                if (!maybe_raw_instr.has_value()) [[unlikely]]
                {
                    raise_exception(maybe_raw_instr.error(), pc_);
                    goto exception;
                }
                const auto &instr = bb.emplace_back(Decoder::decode(*maybe_raw_instr));
                if (!execute(instr)) [[unlikely]]
                    goto exception;
                ++instr_count;
                if (instr.is_terminator())
                    break;
            }

            bb_cache_.update(bb_pc, std::move(bb));
            bb.clear(); // just in case: moved-from object is in valid but unspecified state
        }
    }

    return instr_count;
}

} // namespace yarvs
