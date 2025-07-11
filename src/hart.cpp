#include <cstdint>
#include <ranges>
#include <system_error>
#include <utility>

#include <fmt/core.h>

#include "yarvs/common.hpp"
#include "yarvs/decoder.hpp"
#include "yarvs/hart.hpp"

#include "yarvs/privileged/machine/misa.hpp"

namespace yarvs
{

Hart::Hart()
    : priv_level_{PrivilegeLevel::kMachine}, mem_{csrs_, priv_level_},
      bb_cache_{kDefaultCacheCapacity}
{
    csrs_.set_misa(MISA::Extensions::kI | MISA::Extensions::kS | MISA::Extensions::kU);
}

void Hart::set_log_file(std::string_view file_name)
{
    if (file_name.empty())
        log_file_.reset(stderr);
    else if (FILE *file = std::fopen(file_name.data(), "w"))
        log_file_.reset(file);
    else
    {
        logging_ = false;
        throw std::system_error{errno, std::system_category(), "std::fopen() failed"};
    }
}

bool Hart::execute(const Instruction &instr)
{
    if (!logging_)
        return kCallbacks_[instr.id](*this, instr);

    fmt::println(log_file_.get(), "[{:#010x}]: {}", pc_, instr.disassemble());
    if (instr.id == InstrID::kECALL)
    {
        fmt::println(log_file_.get(), "    syscall:  {}", gprs_.get_reg(kSyscallNumReg));
        for (const auto i : kSyscallArgRegs) {
            fmt::println(log_file_.get(), "    x{}:      {:#x}", i, gprs_.get_reg(i));
        }
    }

    const auto old_gprs_ = gprs_;

    const bool res = kCallbacks_[instr.id](*this, instr);

    auto diff_view = std::views::zip(std::views::iota(0uz), old_gprs_, gprs_)
                   | std::views::filter([](const auto &t){
                         return std::get<1>(t) != std::get<2>(t);
                     });

    for (const auto [i, before, after] : diff_view) {
        fmt::println(log_file_.get(), "    x{}: {:#x} -> {:#x}", i, before, after);
    }

    return res;
}

bool Hart::run_single() {
    run_ = true;

    if (const auto raw_instr_or_err = mem_.fetch(pc_); !raw_instr_or_err.has_value()) [[unlikely]]
    {
        raise_exception(raw_instr_or_err.error(), pc_);
        return false;
    }
    else if (!execute(Decoder::decode(*raw_instr_or_err))) [[unlikely]]
        return false;
    else if (!run_)
        return false;
    return true;
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
                const auto raw_instr_or_err = mem_.fetch(pc_);
                if (!raw_instr_or_err.has_value()) [[unlikely]]
                {
                    raise_exception(raw_instr_or_err.error(), pc_);
                    goto exception;
                }
                const auto &instr = bb.emplace_back(Decoder::decode(*raw_instr_or_err));
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
