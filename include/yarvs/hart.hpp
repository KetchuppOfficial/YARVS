#ifndef INCLUDE_HART_HPP
#define INCLUDE_HART_HPP

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#include "yarvs/common.hpp"
#include "yarvs/instruction.hpp"
#include "yarvs/reg_file.hpp"

#include "yarvs/cache/lru.hpp"

#include "yarvs/memory/memory.hpp"

#include "yarvs/privileged/cs_regfile.hpp"

#include "yarvs/privileged/machine/mcause.hpp"
#include "yarvs/privileged/machine/mstatus.hpp"

#include "yarvs/privileged/supervisor/scause.hpp"
#include "yarvs/privileged/supervisor/sstatus.hpp"

namespace yarvs
{

class Hart final
{
public:

    static constexpr std::size_t kSP = 2;
    static constexpr std::size_t kSyscallRetReg = 10;
    static constexpr std::array<std::size_t, 6> kSyscallArgRegs = {10, 11, 12, 13, 14, 15};
    static constexpr std::size_t kSyscallNumReg = 17;

    explicit Hart();

    // returns the number of executed instructions
    std::uintmax_t run();

    // returns false if an exception was raised
    bool run_single();

    DoubleWord get_pc() const noexcept { return pc_; }
    void set_pc(DoubleWord pc) noexcept { pc_ = pc; }

    RegFile &gprs() noexcept { return gprs_; }
    const RegFile &gprs() const noexcept { return gprs_; }

    CSRegFile &csrs() noexcept { return csrs_; }
    const CSRegFile &csrs() const noexcept { return csrs_; }

    Memory &memory() noexcept { return mem_; }
    const Memory &memory() const noexcept { return mem_; }

    int get_status() const noexcept { return status_; }

    bool logging_enabled() const noexcept { return logging_; }
    void set_logging(bool logging) noexcept { logging_ = logging; }
    void set_log_file(std::string_view file_name);

    PrivilegeLevel get_privilege_level() const noexcept { return priv_level_; }

private:

    void raise_exception(DoubleWord cause, DoubleWord info) noexcept
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

    PrivilegeLevel eh_mode(DoubleWord cause) const noexcept
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

    #include "yarvs/executor_declarations.hpp" // generated header

    bool execute(const Instruction &instr);

    template<std::regular_invocable<DoubleWord, DoubleWord> F>
    void exec_rvi_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        gprs_.set_reg(instr.rd, bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2)));
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
    void exec_rv64i_reg_reg(const Instruction &instr, F bin_op)
    noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
    {
        auto res = bin_op(gprs_.get_reg(instr.rs1), gprs_.get_reg(instr.rs2));
        gprs_.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
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

    template<typename F>
    bool exec_csrrw_csrrwi(const Instruction &instr, F rhs)
    {
        if (priv_level_ < CSRegFile::get_lowest_privilege_level(instr.imm) ||
            CSRegFile::is_for_debug_mode(instr.imm) ||
            CSRegFile::is_read_only(instr.imm)) [[unlikely]]
        {
            raise_exception(MCause::kIllegalInstruction, instr.raw);
            return false;
        }

        if (instr.rd == 0)
            csrs_.set_reg(instr.imm, std::invoke(rhs, instr));
        else
        {
            auto csr = csrs_.get_reg(instr.imm);
            csrs_.set_reg(instr.imm, std::invoke(rhs, instr));
            gprs_.set_reg(instr.rd, csr);
        }

        pc_ += sizeof(RawInstruction);
        return true;
    }

    template<std::regular_invocable<DoubleWord, DoubleWord> F, typename T>
    bool exec_csrrs_csrrc(const Instruction &instr, F bin_op, T rhs)
    {
        if (priv_level_ < CSRegFile::get_lowest_privilege_level(instr.imm) ||
            CSRegFile::is_for_debug_mode(instr.imm)) [[unlikely]]
        {
            raise_exception(MCause::kIllegalInstruction, instr.raw);
            return false;
        }

        if (instr.rs1 == 0)
            gprs_.set_reg(instr.rd, csrs_.get_reg(instr.imm));
        else
        {
            if (CSRegFile::is_read_only(instr.imm)) [[unlikely]]
            {
                raise_exception(MCause::kIllegalInstruction, instr.raw);
                return false;
            }

            const auto csr = csrs_.get_reg(instr.imm);
            csrs_.set_reg(instr.imm, bin_op(csr, std::invoke(rhs, instr)));
            gprs_.set_reg(instr.rd, csr);
        }

        pc_ += sizeof(RawInstruction);
        return true;
    }

    PrivilegeLevel priv_level_ = PrivilegeLevel::kMachine;

    RegFile gprs_;
    DoubleWord pc_;
    CSRegFile csrs_;

    Memory mem_;

    static constexpr std::size_t kDefaultCacheCapacity = 64;
    static constexpr std::size_t kDefaultBBLength = 24;
    using BasicBlock = std::vector<Instruction>;
    LRU<DoubleWord, BasicBlock> bb_cache_;

    int status_ = 0;
    bool run_ = false;

    bool logging_ = false;

    struct LoggerDeleter
    {
        void operator()(FILE *file) const noexcept
        {
            if (file != stderr)
                std::fclose(file);
        }
    };

    std::unique_ptr<FILE, LoggerDeleter> log_file_;
};

} // namespace yarvs

#endif // INCLUDE_HART_HPP
