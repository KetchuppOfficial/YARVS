#include <filesystem>
#include <string>
#include <cstddef>
#include <utility>
#include <chrono>
#include <exception>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "config.hpp"
#include "hart.hpp"

#include "supervisor/satp.hpp"

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

    std::string translation_mode_str;
    app.add_option("--translation-mode", translation_mode_str,
                   "Mode of virtual to physical address translation")
        ->check(CLI::IsMember({"Sv39", "Sv48", "Sv57"}))
        ->default_val("Sv48");

    std::size_t n_stack_pages;
    app.add_option("--n-stack-pages", n_stack_pages, "The number of 4KB pages reserved for stack")
        ->check(CLI::PositiveNumber)
        ->default_val(4);

    CLI11_PARSE(app, argc, argv);

    auto translation_mode = [&]
    {
        if (translation_mode_str == "Sv39")
            return yarvs::SATP::Mode::kSv39;
        else if (translation_mode_str == "Sv48")
            return yarvs::SATP::Mode::kSv48;
        else if (translation_mode_str == "Sv57")
            return yarvs::SATP::Mode::kSv57;
        else
            std::unreachable();
    }();

    yarvs::Config config{translation_mode, n_stack_pages};
    yarvs::Hart hart{config, elf_path};

    auto start = std::chrono::high_resolution_clock::now();
    auto instr_count = hart.run();
    auto finish = std::chrono::high_resolution_clock::now();

    using mcs = std::chrono::microseconds;
    auto time = std::chrono::duration_cast<mcs>(finish - start).count();

    if (perf)
        fmt::println("Executed {} instructions in {} mcs.\nPerformance: {:.2f} MIPS",
                     instr_count, time, static_cast<double>(instr_count) / time);

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
