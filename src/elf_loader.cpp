#include <string>
#include <stdexcept>
#include <utility>
#include <ranges>

#include <fmt/format.h>

#include "common.hpp"
#include "elf_loader.hpp"

namespace yarvs
{

LoadableImage::LoadableImage(const std::string &path_str)
{
    ELFIO::elfio elf;
    if (!elf.load(path_str))
        throw std::runtime_error{fmt::format("could not load ELF file \"{}\"", path_str)};

    if (auto error_msg = elf.validate(); !error_msg.empty())
        throw std::invalid_argument{fmt::format("ELF file is invalid: {}", error_msg)};

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

        throw std::invalid_argument{fmt::format("ELF is of type \"{}\"; executable expected",
                                    type_str)};
    }

    if (elf.get_machine() != ELFIO::EM_RISCV)
        throw std::invalid_argument{"only RISC-V executables are supported"};

    auto is_loadable = [](auto &seg){ return seg->get_type() == ELFIO::PT_LOAD; };

    for (auto &seg : std::views::filter(elf.segments, is_loadable))
    {
        emplace_back(reinterpret_cast<const Byte *>(seg->get_data()), seg->get_file_size(),
                     seg->get_memory_size(), seg->get_virtual_address(), seg->get_flags());
    }

    entry_ = elf.get_entry();
}

} // namespace yarvs
