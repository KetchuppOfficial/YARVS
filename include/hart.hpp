#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <stdexcept>
#include <array>
#include <utility>
#include <vector>
#include <concepts>
#include <type_traits>

#include "common.hpp"
#include "instruction.hpp"
#include "reg_file.hpp"
#include "config.hpp"

#include "memory/memory.hpp"

#include "cache/lru.hpp"

#include "privileged/cs_regfile.hpp"

#include "privileged/supervisor/scause.hpp"

namespace yarvs
{

class Hart final
{
public:

    static constexpr std::size_t kSyscallRetReg = 10;
    static constexpr std::array<std::size_t, 6> kSyscallArgRegs = {10, 11, 12, 13, 14, 15};
    static constexpr std::size_t kSyscallNumReg = 17;

    explicit Hart();
    explicit Hart(const Config &config, const std::filesystem::path &path);

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

    PrivilegeLevel get_privilege_level() const noexcept { return priv_level_; }

private:

    void load_elf(const Config &config, const std::filesystem::path &path);

    [[noreturn]] static void default_exception_handler(CSRegfile &csrs)
    {
        auto what = csrs.get_scause().what();
        if (!what)
            what = "unknown exception";
        throw std::runtime_error{
            std::format("exception \"{}\" at address {:#x} while executing instruction at PC {:#x}",
                        what, csrs.get_stval(), csrs.get_sepc())};
    }

    [[noreturn]] void raise_exception(DoubleWord info, SCause::Exception e)
    {
        csrs_.set_sepc(pc_);
        SCause scause;
        scause.set_exception(e);
        csrs_.set_scause(scause);
        csrs_.set_stval(info);
        auto handler = reinterpret_cast<exception_handler_type>(csrs_.get_stvec().get_base());
        handler(csrs_);
        std::unreachable();
    }

    using exception_handler_type = std::decay_t<decltype(default_exception_handler)>;

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
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.load<T>(va);
        if (!maybe_value.has_value()) [[unlikely]]
            raise_exception(va, maybe_value.error());
        gprs_.set_reg(instr.rd, sext<kNBits<T>, DoubleWord>(*maybe_value));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<typename T>
    requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
    bool exec_uload(const Instruction &instr)
    {
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.load<T>(va);
        if (!maybe_value.has_value()) [[unlikely]]
            raise_exception(va, maybe_value.error());
        gprs_.set_reg(instr.rd, static_cast<DoubleWord>(*maybe_value));
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<riscv_type T>
    bool exec_store(const Instruction &instr)
    {
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.store(va, static_cast<T>(gprs_.get_reg(instr.rs2)));
        if (!maybe_value.has_value()) [[unlikely]]
            raise_exception(va, maybe_value.error());
        pc_ += sizeof(RawInstruction);
        return false;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_zicsr_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto csr = csrs_.get_reg(instr.csr);
        csrs_.set_reg(instr.csr, bin_op(csr, gprs_.get_reg(instr.rs1)));
        gprs_.set_reg(instr.rd, csr);
        return false;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    bool exec_zicsr_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto csr = csrs_.get_reg(instr.csr);
        csrs_.set_reg(instr.csr, bin_op(csr, instr.imm));
        gprs_.set_reg(instr.rd, csr);
        return false;
    }

    PrivilegeLevel priv_level_ = PrivilegeLevel::kMachine;

    static constexpr DoubleWord kSP = 2;
    RegFile gprs_;
    DoubleWord pc_;
    CSRegfile csrs_;

    static constexpr DoubleWord kPPN = 1;
    static constexpr DoubleWord kFreePhysMemBegin = Memory::kPhysMemAmount / 4;
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
