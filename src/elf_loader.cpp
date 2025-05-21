#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <fmt/std.h>

#include <elfio/elfio.hpp>

#include "yarvs/common.hpp"
#include "yarvs/elf_loader.hpp"

#include "yarvs/memory/memory.hpp"

namespace yarvs
{

struct ELFLoader::ELFParser final : public ELFIO::elfio {};

ELFLoader::ELFLoader(const std::filesystem::path &path) : elf_{std::make_unique<ELFParser>()}
{
    if (!elf_->load(path.native()))
        throw std::runtime_error{fmt::format("could not load ELF file \"{}\"", path)};

    if (const auto error_msg = elf_->validate(); !error_msg.empty())
        throw std::invalid_argument{fmt::format("ELF file is invalid: {}", error_msg)};

    if (elf_->get_class() != ELFIO::ELFCLASS64)
        throw std::invalid_argument{"only 64-bit ELF files are supported"};

    if (const auto type = elf_->get_type(); type != ELFIO::ET_EXEC)
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

        throw std::invalid_argument{fmt::format("ELF is of type \"{}\"; executable expected",
                                    type_str)};
    }

    if (elf_->get_machine() != ELFIO::EM_RISCV)
        throw std::invalid_argument{"only RISC-V executables are supported"};
}

ELFLoader::~ELFLoader() = default;

DoubleWord ELFLoader::get_entry() const
{
    static_assert(std::is_same_v<DoubleWord, ELFIO::Elf64_Addr>);
    return elf_->get_entry();
}

std::size_t ELFLoader::segments_count() const { return elf_->segments.size(); }

ELFLoader::Segment ELFLoader::segment(std::size_t i) const
{
    const auto *segment = elf_->segments[i];
    return Segment{.data = reinterpret_cast<const unsigned char *>(segment->get_data()),
                   .memory_size = segment->get_memory_size(),
                   .file_size = segment->get_file_size(),
                   .virtual_address = segment->get_virtual_address(),
                   .loadable = segment->get_type() == ELFIO::PT_LOAD};
}

std::map<DoubleWord, ELFLoader::SegmentFlags> ELFLoader::get_loadable_pages() const
{
    static_assert(std::is_same_v<std::underlying_type_t<SegmentFlags>, ELFIO::Elf_Word>);

    std::map<DoubleWord, SegmentFlags> pages;
    auto is_loadable = [](const auto &seg){ return seg->get_type() == ELFIO::PT_LOAD; };
    for (const auto &seg : std::views::filter(elf_->segments, is_loadable))
    {
        const auto va = seg->get_virtual_address();
        const auto last_page_addr =
            mask_bits<63, Memory::kPageBits>(va + seg->get_memory_size());

        for (auto first_page_addr = mask_bits<63, Memory::kPageBits>(va);
             first_page_addr <= last_page_addr; first_page_addr += Memory::kPageSize)
            pages.emplace(first_page_addr, SegmentFlags{seg->get_flags()});
    }

    return pages;
}

} // namespace yarvs
