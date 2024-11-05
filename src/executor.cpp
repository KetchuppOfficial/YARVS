#include <concepts>
#include <functional>
#include <stdexcept>
#include <type_traits>

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

void exec_add(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::plus{});
}

void exec_sub(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::minus{});
}

void exec_and(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_and{});
}

void exec_xor(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_xor{});
}
void exec_or(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::bit_or{});
}

void exec_sltu(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, std::less{});
}

void exec_slt(const Instruction &instr, Hart &h) noexcept
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

void exec_addw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_reg(instr, h, std::plus{});
}

void exec_subw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_reg(instr, h, std::minus{});
}

void exec_sll(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<5, 0>(rhs); });
}

void exec_srl(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<5, 0>(rhs); });
}

void exec_sra(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

void exec_sllw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<4, 0>(rhs); });
}

void exec_srlw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<4, 0>(rhs); });
}

void exec_sraw(const Instruction &instr, Hart &h) noexcept
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

void exec_addi(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, std::plus{});
}

void exec_andi(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_and{});
}

void exec_ori(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_or{});
}

void exec_xori(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, std::bit_xor{});
}

void exec_sltiu(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, std::less{});
}

void exec_slti(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

void exec_lui(const Instruction &instr, Hart &h) noexcept
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, instr.imm);
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_auipc(const Instruction &instr, Hart &h) noexcept
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

void exec_addiw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_imm(instr, h, std::plus{});
}

void exec_slli(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<5, 0>(rhs); });
}

void exec_srli(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<5, 0>(rhs); });
}

void exec_srai(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

void exec_slliw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

void exec_srliw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

void exec_sraiw(const Instruction &instr, Hart &h) noexcept
{
    exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<5, 0>(rhs);
    });
}

// RVI control transfer instructions

void exec_jal(const Instruction &instr, Hart &h) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + sizeof(RawInstruction));
    h.set_pc(h.get_pc() + instr.imm);
}

void exec_jalr(const Instruction &instr, Hart &h) noexcept
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

void exec_beq(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, std::equal_to{});
}

void exec_bne(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, std::not_equal_to{});
}

void exec_blt(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

void exec_bltu(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, std::less{});
}

void exec_bge(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >= to_signed(rhs);
    });
}

void exec_bgeu(const Instruction &instr, Hart &h) noexcept
{
    exec_cond_branch(instr, h, std::greater_equal{});
}

// RV64I load and store instructions

template<riscv_type T>
void exec_load(const Instruction &instr, Hart &h) noexcept
{
    auto value = h.memory().load<T>(instr.rs1 + instr.imm);
    h.gprs().set_reg(instr.rd, sext<sizeof(T), DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_ld(const Instruction &instr, Hart &h) noexcept { exec_load<DoubleWord>(instr, h); }
void exec_lw(const Instruction &instr, Hart &h) noexcept { exec_load<Word>(instr, h); }
void exec_lh(const Instruction &instr, Hart &h) noexcept { exec_load<HalfWord>(instr, h); }
void exec_lb(const Instruction &instr, Hart &h) noexcept { exec_load<Byte>(instr, h); }

template<typename T>
requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
void exec_uload(const Instruction &instr, Hart &h) noexcept
{
    auto value = h.memory().load<T>(instr.rs1 + instr.imm);
    h.gprs().set_reg(instr.rd, static_cast<DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_lwu(const Instruction &instr, Hart &h) noexcept { exec_uload<Word>(instr, h); }
void exec_lhu(const Instruction &instr, Hart &h) noexcept { exec_uload<HalfWord>(instr, h); }
void exec_lbu(const Instruction &instr, Hart &h) noexcept { exec_uload<Byte>(instr, h); }

template<riscv_type T>
void exec_store(const Instruction &instr, Hart &h) noexcept
{
    h.memory().store(instr.rs1 + instr.imm, static_cast<T>(h.gprs().get_reg(instr.rs2)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
}

void exec_sd(const Instruction &instr, Hart &h) noexcept { exec_store<DoubleWord>(instr, h); }
void exec_sw(const Instruction &instr, Hart &h) noexcept { exec_store<Word>(instr, h); }
void exec_sh(const Instruction &instr, Hart &h) noexcept { exec_store<HalfWord>(instr, h); }
void exec_sb(const Instruction &instr, Hart &h) noexcept { exec_store<Byte>(instr, h); }

// RVI memory ordering instructions

void exec_fence(const Instruction &instr, Hart &h)
{
    throw std::runtime_error{"not implemented"};
}

// RVI environment call and breakpoints

void exec_ecall(const Instruction &instr, Hart &h)
{
    throw std::runtime_error{"not implemented"};
}

void exec_ebreak(const Instruction &instr, Hart &h)
{
    throw std::runtime_error{"not implemented"};
}

} // namespace yarvs

#include "executor_table.hpp"
