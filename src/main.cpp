#include <exception>
#include <filesystem>

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
    hart.run();

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
