#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <stdexcept>
#include <array>
#include <vector>
#include <concepts>
#include <type_traits>

#include "common.hpp"
#include "instruction.hpp"
#include "reg_file.hpp"
#include "config.hpp"

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

    explicit Hart(const Config &config);
    explicit Hart(const Config &config, const std::filesystem::path &path);

    void load_elf(const std::filesystem::path &path);

    // returns the number of executed instructions
    std::uintmax_t run();

    Word get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    RegFile &gprs() noexcept { return gprs_; }
    const RegFile &gprs() const noexcept { return gprs_; }

    Memory &memory() noexcept { return mem_; }
    const Memory &memory() const noexcept { return mem_; }

    int get_status() const noexcept { return status_; }
    void set_status(int status) noexcept { status_ = status; }

private:

    [[noreturn]] static void default_exception_handler(CSRegfile &csrs)
    {
        throw std::runtime_error{csrs.scause.what()};
    }

    /*
     * Pointer to static method is used instead of pointer to non-static method, because:
     * 1) the size of a pointer to static method is usually less than the size of a pointer to a
     *    non-static method;
     * 2) calling through pointer to non-static method involved run-time checking on whether this
     *    pointer points to a virtual function.
     */
    using callback_type = bool(*)(Hart &, const Instruction &);

    #include "executor_declarations.hpp"

    bool execute(const Instruction &instr) { return (kCallbacks_[instr.id])(*this, instr); }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_rvi_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        gprs_.set_reg(instr.rd, bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2)));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_rv64i_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto res = bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2));
        gprs_.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_rvi_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        gprs_.set_reg(instr.rd, bin_op(gprs_.get_reg(instr.rs1), instr.imm));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_rv64i_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto res = bin_op(gprs_.get_reg(instr.rs1), instr.imm);
        gprs_.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<std::predicate<DoubleWord, DoubleWord> F>
    bool exec_cond_branch(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        if (bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2)))
            pc_ += instr.imm;
        else
            pc_ += sizeof(RawInstruction);
        return true;
    }

    template<riscv_type T>
    bool exec_load(const Instruction &instr)
    {
        auto value = mem_.load<T>(gprs_.get_reg(instr.rs1) + instr.imm);
        gprs_.set_reg(instr.rd, sext<kNBits<T>, DoubleWord>(value));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<typename T>
    requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
    bool exec_uload(const Instruction &instr)
    {
        auto value = mem_.load<T>(gprs_.get_reg(instr.rs1) + instr.imm);
        gprs_.set_reg(instr.rd, static_cast<DoubleWord>(value));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<riscv_type T>
    bool exec_store(const Instruction &instr)
    {
        mem_.store(gprs_.get_reg(instr.rs1) + instr.imm, static_cast<T>(gprs_.get_reg(instr.rs2)));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    static constexpr DoubleWord kSP = 2;
    RegFile gprs_;
    DoubleWord pc_;
    CSRegfile csrs_;

    static constexpr DoubleWord kPPN = 1;
    static constexpr DoubleWord kFreePhysMemBegin = Memory::kPhysMemAmount / 4;
    Memory mem_;

    Config config_;

    static constexpr std::size_t kDefaultCacheCapacity = 64;
    static constexpr std::size_t kDefaultBBLength = 24;
    using BasicBlock = std::vector<Instruction>;
    LRU<DoubleWord, BasicBlock> bb_cache_;

    int status_ = 0;
    bool run_ = false;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
