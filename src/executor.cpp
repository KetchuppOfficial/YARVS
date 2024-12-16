#include <concepts>
#include <functional>
#include <stdexcept>
#include <type_traits>

#include <unistd.h>

#include "hart.hpp"
#include "common.hpp"
#include "executor.hpp"
#include "instruction.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// RVI integer register-register operations

template<std::regular_invocable<DoubleWord, DoubleWord> F>
void exec_rvi_reg_reg(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_add(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::plus{});
}

void exec_sub(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::minus{});
}

void exec_and(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_and{});
}

void exec_xor(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_xor{});
}
void exec_or(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_or{});
}

void exec_sltu(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, std::less{});
}

void exec_slt(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return to_signed(lhs) < to_signed(rhs); });
}

// RVI64 integer register-register operations

template<std::regular_invocable<DoubleWord, DoubleWord> F>
void exec_rv64i_reg_reg(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    auto res = bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2));
    gprs.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_addw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_reg(instr, h, std::plus{});
}

void exec_subw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_reg(instr, h, std::minus{});
}

void exec_sll(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<5, 0>(rhs); });
}

void exec_srl(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<5, 0>(rhs); });
}

void exec_sra(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

void exec_sllw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<4, 0>(rhs); });
}

void exec_srlw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<4, 0>(rhs); });
}

void exec_sraw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<4, 0>(rhs);
    });
}

// RVI integer register-immediate instructions

template<std::regular_invocable<DoubleWord, DoubleWord> F>
void exec_rvi_reg_imm(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), instr.imm));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_addi(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, std::plus{});
}

void exec_andi(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_and{});
}

void exec_ori(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_or{});
}

void exec_xori(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_xor{});
}

void exec_sltiu(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, std::less{});
}

void exec_slti(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

void exec_lui(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, instr.imm);
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_auipc(Hart &h, const Instruction &instr) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + instr.imm);
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

// RV64I integer register-immediate instructions

template<std::regular_invocable<DoubleWord, DoubleWord> F>
void exec_rv64i_reg_imm(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    auto res = bin_op(gprs.get_reg(instr.rs1), instr.imm);
    gprs.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_addiw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_imm(instr, h, std::plus{});
}

void exec_slli(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<5, 0>(rhs); });
}

void exec_srli(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<5, 0>(rhs); });
}

void exec_srai(Hart &h, const Instruction &instr) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

void exec_slliw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

void exec_srliw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

void exec_sraiw(Hart &h, const Instruction &instr) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<5, 0>(rhs);
    });
}

// RVI control transfer instructions

void exec_jal(Hart &h, const Instruction &instr) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + sizeof(RawInstruction));
    h.set_pc(h.get_pc() + instr.imm);
}

void exec_jalr(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, h.get_pc() + sizeof(RawInstruction));
    h.set_pc((gprs.get_reg(instr.rs1) + instr.imm) & ~DoubleWord{1});
}

template<std::predicate<DoubleWord, DoubleWord> F>
void exec_cond_branch(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    const auto &gprs = h.gprs();
    if (bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2)))
        h.set_pc(h.get_pc() + instr.imm);
    else
        h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_beq(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, std::equal_to{});
}

void exec_bne(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, std::not_equal_to{});
}

void exec_blt(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

void exec_bltu(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, std::less{});
}

void exec_bge(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >= to_signed(rhs);
    });
}

void exec_bgeu(Hart &h, const Instruction &instr) noexcept
{
    exec_cond_branch(instr, h, std::greater_equal{});
}

// RV64I load and store instructions

template<riscv_type T>
void exec_load(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    auto value = h.memory().load<T>(gprs.get_reg(instr.rs1) + instr.imm);
    gprs.set_reg(instr.rd, sext<kNBits<T>, DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_ld(Hart &h, const Instruction &instr) noexcept { exec_load<DoubleWord>(h, instr); }
void exec_lw(Hart &h, const Instruction &instr) noexcept { exec_load<Word>(h, instr); }
void exec_lh(Hart &h, const Instruction &instr) noexcept { exec_load<HalfWord>(h, instr); }
void exec_lb(Hart &h, const Instruction &instr) noexcept { exec_load<Byte>(h, instr); }

template<typename T>
requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
void exec_uload(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    auto value = h.memory().load<T>(gprs.get_reg(instr.rs1) + instr.imm);
    gprs.set_reg(instr.rd, static_cast<DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_lwu(Hart &h, const Instruction &instr) noexcept { exec_uload<Word>(h, instr); }
void exec_lhu(Hart &h, const Instruction &instr) noexcept { exec_uload<HalfWord>(h, instr); }
void exec_lbu(Hart &h, const Instruction &instr) noexcept { exec_uload<Byte>(h, instr); }

template<riscv_type T>
void exec_store(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    h.memory().store(gprs.get_reg(instr.rs1) + instr.imm, static_cast<T>(gprs.get_reg(instr.rs2)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_sd(Hart &h, const Instruction &instr) noexcept { exec_store<DoubleWord>(h, instr); }
void exec_sw(Hart &h, const Instruction &instr) noexcept { exec_store<Word>(h, instr); }
void exec_sh(Hart &h, const Instruction &instr) noexcept { exec_store<HalfWord>(h, instr); }
void exec_sb(Hart &h, const Instruction &instr) noexcept { exec_store<Byte>(h, instr); }

// RVI memory ordering instructions

[[noreturn]] void exec_fence(Hart &h, const Instruction &instr)
{
    throw std::runtime_error{"FENCE is not implemented"};
}

// RVI environment call and breakpoints

void exec_ecall(Hart &h, [[maybe_unused]] const Instruction &instr)
{
    switch (auto syscall_num = h.gprs().get_reg(Hart::kSyscallNumReg))
    {
        case 64: // write
        {
            auto &gprs = h.gprs();
            auto fd = gprs.get_reg(Hart::kSyscallArgRegs[0]);
            auto *ptr = h.memory().host_ptr(gprs.get_reg(Hart::kSyscallArgRegs[1]));
            auto size = gprs.get_reg(Hart::kSyscallArgRegs[2]);

            auto res = write(fd, reinterpret_cast<const char *>(ptr), size);

            gprs.set_reg(Hart::kSyscallRetReg, res);
            h.set_pc(h.get_pc() + sizeof(RawInstruction));
            break;
        }
        case 93: // exit
            h.stop();
            h.set_status(h.gprs().get_reg(Hart::kSyscallRetReg));
            break;
        default:
            throw std::runtime_error{std::format("System call {:#x} is not supported",
                                                 syscall_num)};
    }
}

void exec_ebreak(Hart &h, [[maybe_unused]] const Instruction &instr) noexcept { h.stop(); }

} // namespace yarvs

#include "executor_table.hpp"
