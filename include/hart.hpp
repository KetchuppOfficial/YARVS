#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <vector>

#include "common.hpp"
#include "instruction.hpp"
#include "reg_file.hpp"
#include "executor.hpp"

#include "memory/memory.hpp"

#include "cache/lru.hpp"

#include "supervisor/cs_regfile.hpp"

namespace yarvs
{

class Hart final
{
public:

    static constexpr std::size_t kSyscallRetReg = 10;
    static constexpr std::array<std::size_t, 6> kSyscallArgRegs = {10, 11, 12, 13, 14, 15};
    static constexpr std::size_t kSyscallNumReg = 17;

    explicit Hart();
    explicit Hart(const std::filesystem::path &path);

    void load_elf(const std::filesystem::path &path);

    // returns the number of executed instructions
    std::uintmax_t run();

    void stop() noexcept { run_ = false; };

    Word get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    RegFile &gprs() noexcept { return reg_file_; }
    const RegFile &gprs() const noexcept { return reg_file_; }

    Memory &memory() noexcept { return mem_; }
    const Memory &memory() const noexcept { return mem_; }

    int get_status() const noexcept { return status_; }
    void set_status(int status) noexcept { status_ = status; }

private:

    static void default_exception_handler()
    {
        //
    }

    static constexpr DoubleWord kSP = 2;
    RegFile reg_file_;
    DoubleWord pc_;
    CSRegfile csrs_;

    static constexpr DoubleWord kPPN = 1;
    static constexpr DoubleWord kFreePhysMemBegin = Memory::kPhysMemAmount / 4;
    static constexpr DoubleWord kStackAddr = 0x7ffe49b14000;
    static_assert((kStackAddr / Memory::kPageSize) * Memory::kPageSize == kStackAddr);
    Memory mem_;

    static constexpr std::size_t kDefaultCacheCapacity = 64;
    static constexpr std::size_t kDefaultBBLength = 24;
    using BasicBlock = std::vector<Instruction>;
    LRU<DoubleWord, BasicBlock> bb_cache_;

    int status_ = 0;
    bool run_ = false;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
