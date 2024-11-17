#include <exception>
#include <filesystem>
#include <chrono>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "hart.hpp"

int main(int argc, char **argv) try
{
    CLI::App app{"YARVS: Yet Another RISC-V Simulator"};

    std::filesystem::path elf_path;
    app.add_option("elf", elf_path, "Path to RISC-V executable")
        ->required()
        ->check(CLI::ExistingFile);

    bool perf = false;
    app.add_flag("--perf", perf, "Measure performance: execution time, "
                                 "the number of executed instructions and MIPS");

    CLI11_PARSE(app, argc, argv);

    yarvs::Hart hart{elf_path};

    if (perf)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto instr_count = hart.run();
        auto finish = std::chrono::high_resolution_clock::now();

        using ms = std::chrono::microseconds;
        auto time = std::chrono::duration_cast<ms>(finish - start).count();

        fmt::println("\nExecuted {} instructions in {} mcs", instr_count, time);
        fmt::println("Performance: {} MIPS", instr_count * 1e3 / time);
    }
    else
        hart.run();

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
