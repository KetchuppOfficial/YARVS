#include <exception>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "elf_loader.hpp"

int main(int argc, char **argv) try
{
    CLI::App app{"YARVS: Yet Another RISC-V Simulator"};

    std::filesystem::path elf_path;
    app.add_option("--elf", elf_path, "Path to RISC-V executable")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    yarvs::LoadableImage segments{elf_path};

    auto i = 0;
    fmt::println("ELF file contains {} loadable segments", segments.size());
    for (auto &seg : segments)
    {
        fmt::println("Segment {}", i++);
        fmt::println("    file size: {} bytes", seg.get_file_size());
        fmt::println("    memory size: {} bytes", seg.get_memory_size());
        fmt::println("    virtual address: {:#x}", seg.get_virtual_addr());
    }

    return 0;
}
catch (const std::exception &e)
{
    fmt::println("Caught as instance of {}.\nwhat(): {}", typeid(e).name(), e.what());
    return 1;
}
catch (...)
{
    fmt::println("Caught an unknown exception");
    return 1;
}
