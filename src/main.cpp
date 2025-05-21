#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <ranges>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>

#include <fmt/format.h>

#include "yarvs/bits_manipulation.hpp"
#include "yarvs/common.hpp"
#include "yarvs/elf_loader.hpp"
#include "yarvs/hart.hpp"

#include "yarvs/privileged/xtvec.hpp"

#include "yarvs/privileged/supervisor/satp.hpp"

namespace {

yarvs::DoubleWord get_initial_sp(yarvs::SATP::Mode translation_mode)
{
    yarvs::DoubleWord sp;

    switch (translation_mode)
    {
        case yarvs::SATP::Mode::kBare:
            sp = 0x1fffffffffff'000;
            break;
        case yarvs::SATP::Mode::kSv39:
            sp =  0x3ffffff'000;
            assert((yarvs::sext<39, yarvs::DoubleWord>(sp)) == sp &&
                   "Page-fault will occur on accessing memory through such stack pointer");
            break;
        case yarvs::SATP::Mode::kSv48:
            sp =  0x7ffffffff'000;
            assert((yarvs::sext<48, yarvs::DoubleWord>(sp)) == sp &&
                   "Page-fault will occur on accessing memory through such stack pointer");
            break;
        case yarvs::SATP::Mode::kSv57:
            sp = 0xfffffffffff'000;
            assert((yarvs::sext<57, yarvs::DoubleWord>(sp)) == sp &&
                   "Page-fault will occur on accessing memory through such stack pointer");
            break;
        default:
            throw std::invalid_argument{
                fmt::format("translation mode {} is not supported",
                            static_cast<yarvs::DoubleWord>(translation_mode))};
    }

    return sp;
}

auto loadable_pages_to_flags(yarvs::ELFLoader &elf, yarvs::DoubleWord stack_top,
                             yarvs::DoubleWord stack_pages_count)
{
    auto page_addr_to_flags = elf.get_loadable_pages();

    const auto stack_last_page_addr = yarvs::mask_bits<63, yarvs::Memory::kPageBits>(stack_top);
    for (const auto i : std::views::iota(yarvs::DoubleWord{0}, stack_pages_count + 1))
    {
        using enum yarvs::ELFLoader::SegmentFlags;
        page_addr_to_flags.emplace(stack_last_page_addr - i * yarvs::Memory::kPageSize,
                                   kRead | kWrite);
    }

    return page_addr_to_flags;
}

void initialize_hart(yarvs::Hart &hart, const std::filesystem::path &elf_path,
                     yarvs::SATP::Mode translation_mode, std::size_t stack_pages_count)
{
    constexpr yarvs::PTE kPointerToNextLevelPTE = 0b10001;
    static_assert(kPointerToNextLevelPTE.get_U() && kPointerToNextLevelPTE.get_V());

    const auto stack_top = get_initial_sp(translation_mode);
    const auto pt_levels = yarvs::SATP::pt_levels(translation_mode);

    // Set stack pointer
    hart.gprs().set_reg(yarvs::Hart::kSP, stack_top);

    // Set translation mode and PPN of the root page table
    yarvs::SATP satp;
    satp.set_mode(translation_mode);
    constexpr yarvs::DoubleWord kRootPageTablePPN = 1;
    satp.set_ppn(kRootPageTablePPN);
    hart.csrs().set_satp(satp);

    // Load elf from file
    yarvs::ELFLoader elf{elf_path};

    // Set entry point
    hart.set_pc(elf.get_entry());

    // PPN of the currently processed physical page of the page table
    yarvs::DoubleWord table_ppn = kRootPageTablePPN + 1;

    // PPN of the first physical page used for code and data
    yarvs::DoubleWord data_ppn = yarvs::Memory::kPhysMemAmount / (4 * yarvs::Memory::kPageSize);

    // Map addresses of virtual pages to addresses of physical pages for the given ELF
    std::map<yarvs::DoubleWord, yarvs::DoubleWord> va_to_pa;
    for (const auto [page, rwx] : loadable_pages_to_flags(elf, stack_top, stack_pages_count))
    {
        const yarvs::VirtualAddress va = page;
        auto a = kRootPageTablePPN * yarvs::Memory::kPageSize;

        for (yarvs::Byte i = pt_levels - 1; i > 0; --i)
        {
            const auto pa = a + va.get_vpn(i) * sizeof(yarvs::PTE);
            yarvs::PTE pte = hart.memory().load<yarvs::DoubleWord>(pa).value();
            if (pte.get_V())
                a = pte.get_whole_ppn();
            else
            {
                pte = kPointerToNextLevelPTE;
                pte.set_ppn(table_ppn);
                [[maybe_unused]] auto res = hart.memory().store(pa, +pte);
                assert(res.has_value());
                a = table_ppn * yarvs::Memory::kPageSize;
                ++table_ppn;
            }
        }

        using enum yarvs::ELFLoader::SegmentFlags;
        yarvs::PTE pte = kPointerToNextLevelPTE;
        pte.set_R(rwx & kRead);
        pte.set_W(rwx & kWrite);
        pte.set_E(rwx & kExecute);
        pte.set_ppn(data_ppn);

        const auto pa = a + va.get_vpn(0) * sizeof(yarvs::PTE);
        [[maybe_unused]] auto res = hart.memory().store(pa, +pte);
        assert(res.has_value());

        va_to_pa.emplace(page, data_ppn * yarvs::Memory::kPageSize);

        ++data_ppn;
    }

    // Copy contents of the ELF file to the memory of the simulator
    for (const auto i : std::views::iota(0uz, elf.segments_count()))
    {
        const yarvs::ELFLoader::Segment seg = elf.segment(i);
        if (!seg.loadable)
            continue;

        const auto v_page = yarvs::mask_bits<63, yarvs::Memory::kPageBits>(seg.virtual_address);
        const auto pa = va_to_pa.at(v_page) |
                        yarvs::mask_bits<yarvs::Memory::kPageBits - 1, 0>(seg.virtual_address);
        hart.memory().store(pa, seg.data, seg.data + seg.file_size);
    }

    // Set exception handler
    //
    // The address of the trap vector in not placed in the translation tree, because exceptions are
    // handled in M mode where address translation is turned off
    constexpr yarvs::DoubleWord kTrapBaseAddress = 0;
    constexpr std::array<uint32_t, 4> kDefaultExceptionHandler = {
        0x34201573, // csrrw x10, mcause, x0
        0x06450513, // addi x10, x10, 100
        0x05d00893, // addi x17, x0, 93
        0x00000073  // ecall
    };

    yarvs::XTVec mtvec;
    mtvec.set_base(kTrapBaseAddress);
    hart.csrs().set_mtvec(mtvec);
    hart.memory().store(kTrapBaseAddress, kDefaultExceptionHandler.begin(),
                        kDefaultExceptionHandler.end());
}

} // unnamed namespace

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

    std::size_t stack_pages_count;
    app.add_option("--n-stack-pages", stack_pages_count, "The number of 4KB pages reserved for stack")
        ->check(CLI::PositiveNumber)
        ->default_val(4);

    auto *need_logging = app.add_flag("--log", "Enable logging");

    std::string log_file_name;
    auto *log_file_opt = app.add_option("--log-file", log_file_name, "Path to the log file")
        ->default_str("stderr")
        ->needs(need_logging);

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

    yarvs::Hart hart;

    if (*need_logging)
    {
        hart.set_logging(true);
        hart.set_log_file(log_file_name);
    }

    initialize_hart(hart, elf_path, translation_mode, stack_pages_count);

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
