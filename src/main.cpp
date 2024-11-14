#include <exception>
#include <filesystem>
#include <chrono>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "elf_loader.hpp"
#include "hart.hpp"

int main(int argc, char **argv) try
{
    CLI::App app{"YARVS: Yet Another RISC-V Simulator"};

    std::filesystem::path elf_path;
    app.add_option("--elf", elf_path, "Path to RISC-V executable")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    yarvs::LoadableImage segments{elf_path};
    yarvs::Hart hart{segments};

    auto start = std::chrono::high_resolution_clock::now();
    auto instr_count = hart.run();
    auto finish = std::chrono::high_resolution_clock::now();

    using ms = std::chrono::microseconds;
    auto time = std::chrono::duration_cast<ms>(finish - start).count();

    fmt::println("\nExecuted {} instructions in {} mcs", instr_count, time);
    fmt::println("Performance: {} MIPS", instr_count * 1e3 / time);

    return hart.get_status();
}
catch (const std::exception &e)
{
    fmt::println(stderr, "Caught as instance of {}.\nwhat(): {}", typeid(e).name(), e.what());
    return 1;
}
catch (...)
{
    fmt::println(stderr, "Caught an unknown exception");
    return 1;
}
