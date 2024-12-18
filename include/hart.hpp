#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <expected>
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

    PrivilegeLevel get_privilege_level() const noexcept { return priv_level_; }

private:

    void load_elf(const Config &config, const std::filesystem::path &path);

    static constexpr std::array<uint32_t, 4> kDefaultExceptionHandler = {
        0x34201573, // csrrw x10, mcause, x0
        0x06450513, // addi x10, x10, 100
        0x05d00893, // addi x17, x0, 93
        0x00000073  // ecall
    };

    void raise_exception(DoubleWord cause, DoubleWord info)
    {
        if (eh_mode(cause) == PrivilegeLevel::kMachine)
        {
            csrs_.set_mepc(pc_);
            csrs_.set_mtval(info);

            MCause mcause;
            mcause.set_exception(static_cast<MCause::Exception>(cause));
            csrs_.set_mcause(mcause);

            MStatus mstatus = csrs_.get_mstatus();
            mstatus.set_mpp(priv_level_);
            csrs_.set_mstatus(mstatus);

            priv_level_ = PrivilegeLevel::kMachine;
            pc_ = csrs_.get_mtvec().get_base();
        }
        else
        {
            csrs_.set_sepc(pc_);
            csrs_.set_stval(info);

            SCause scause;
            scause.set_exception(static_cast<SCause::Exception>(cause));
            csrs_.set_scause(scause);

            SStatus sstatus = csrs_.get_sstatus();
            sstatus.set_spp(priv_level_);
            csrs_.set_sstatus(sstatus);

            priv_level_ = PrivilegeLevel::kSupervisor;
            pc_ = csrs_.get_stvec().get_base();
        }
    }

    PrivilegeLevel eh_mode(DoubleWord cause) const
    {
        if (priv_level_ == PrivilegeLevel::kMachine)
            return PrivilegeLevel::kMachine;
        if (csrs_.get_medeleg() & (DoubleWord{1} << cause))
            return PrivilegeLevel::kSupervisor;
        return PrivilegeLevel::kMachine;
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

    bool execute(const Instruction &instr)
    {
        return kCallbacks_[instr.id](*this, instr);
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_rvi_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        gprs_.set_reg(instr.rd, bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2)));
        pc_ += sizeof(RawInstruction);
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_rv64i_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto res = bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2));
        gprs_.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
        pc_ += sizeof(RawInstruction);
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_rvi_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        gprs_.set_reg(instr.rd, bin_op(gprs_.get_reg(instr.rs1), instr.imm));
        pc_ += sizeof(RawInstruction);
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_rv64i_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto res = bin_op(gprs_.get_reg(instr.rs1), instr.imm);
        gprs_.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
        pc_ += sizeof(RawInstruction);
    }

    template<std::predicate<DoubleWord, DoubleWord> F>
    void exec_cond_branch(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        if (bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2)))
            pc_ += instr.imm;
        else
            pc_ += sizeof(RawInstruction);
    }

    template<riscv_type T>
    bool exec_load(const Instruction &instr)
    {
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.load<T>(va);
        if (!maybe_value.has_value()) [[unlikely]]
        {
            raise_exception(maybe_value.error(), va);
            return false;
        }
        gprs_.set_reg(instr.rd, sext<kNBits<T>, DoubleWord>(*maybe_value));
        pc_ += sizeof(RawInstruction);
        return true;
    }

    template<typename T>
    requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
    bool exec_uload(const Instruction &instr)
    {
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.load<T>(va);
        if (!maybe_value.has_value()) [[unlikely]]
        {
            raise_exception(maybe_value.error(), va);
            return false;
        }
        gprs_.set_reg(instr.rd, static_cast<DoubleWord>(*maybe_value));
        pc_ += sizeof(RawInstruction);
        return true;
    }

    template<riscv_type T>
    bool exec_store(const Instruction &instr)
    {
        const auto va = gprs_.get_reg(instr.rs1) + instr.imm;
        auto maybe_value = mem_.store(va, static_cast<T>(gprs_.get_reg(instr.rs2)));
        if (!maybe_value.has_value()) [[unlikely]]
        {
            raise_exception(maybe_value.error(), va);
            return false;
        }
        pc_ += sizeof(RawInstruction);
        return true;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_zicsr_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto csr = csrs_.get_reg(instr.csr);
        csrs_.set_reg(instr.csr, bin_op(csr, gprs_.get_reg(instr.rs1)));
        gprs_.set_reg(instr.rd, csr);
        pc_ += sizeof(RawInstruction);
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_zicsr_reg_imm(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto csr = csrs_.get_reg(instr.csr);
        csrs_.set_reg(instr.csr, bin_op(csr, instr.imm));
        gprs_.set_reg(instr.rd, csr);
        pc_ += sizeof(RawInstruction);
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
