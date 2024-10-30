#include <functional>
#include <type_traits>

#include "common.hpp"
#include "executor.hpp" // generated header
#include "instruction.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// RV32I integer register-register operations

template<typename F>
void exec_rvi_reg_reg(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2)));
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

void exec_sll(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs << mask_bits<4, 0>(rhs); });
}

void exec_srl(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs){ return lhs >> mask_bits<4, 0>(rhs); });
}

void exec_sra(const Instruction &instr, Hart &h) noexcept
{
    exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<4, 0>(rhs));
    });
}

// RV32I integer register-immediate instructions

template<typename F>
void exec_rvi_reg_imm(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), instr.imm));
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
}

void exec_auipc(const Instruction &instr, Hart &h) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + instr.imm);
}

} // namespace yarvs
