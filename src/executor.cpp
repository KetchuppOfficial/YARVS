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
bool exec_rvi_reg_reg(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_add(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::plus{});
}

bool exec_sub(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::minus{});
}

bool exec_and(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::bit_and{});
}

bool exec_xor(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::bit_xor{});
}

bool exec_or(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::bit_or{});
}

bool exec_sltu(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, std::less{});
}

bool exec_slt(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

// RVI64 integer register-register operations

template<std::regular_invocable<DoubleWord, DoubleWord> F>
bool exec_rv64i_reg_reg(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    auto res = bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2));
    gprs.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_addw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_reg(instr, h, std::plus{});
}

bool exec_subw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_reg(instr, h, std::minus{});
}

bool exec_sll(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool exec_srl(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool exec_sra(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

bool exec_sllw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<4, 0>(rhs);
    });
}

bool exec_srlw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<4, 0>(rhs);
    });
}

bool exec_sraw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_reg(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<4, 0>(rhs);
    });
}

// RVI integer register-immediate instructions

template<std::regular_invocable<DoubleWord, DoubleWord> F>
bool exec_rvi_reg_imm(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, bin_op(gprs.get_reg(instr.rs1), instr.imm));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_addi(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, std::plus{});
}

bool exec_andi(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, std::bit_and{});
}

bool exec_ori(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, std::bit_or{});
}

bool exec_xori(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, std::bit_xor{});
}

bool exec_sltiu(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, std::less{});
}

bool exec_slti(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

bool exec_lui(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, instr.imm);
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_auipc(Hart &h, const Instruction &instr) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + instr.imm);
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

// RV64I integer register-immediate instructions

template<std::regular_invocable<DoubleWord, DoubleWord> F>
bool exec_rv64i_reg_imm(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    auto &gprs = h.gprs();
    auto res = bin_op(gprs.get_reg(instr.rs1), instr.imm);
    gprs.set_reg(instr.rd, sext<32, DoubleWord>(static_cast<Word>(res)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_addiw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_imm(instr, h, std::plus{});
}

bool exec_slli(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool exec_srli(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool exec_srai(Hart &h, const Instruction &instr) noexcept
{
    return exec_rvi_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

bool exec_slliw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool exec_srliw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool exec_sraiw(Hart &h, const Instruction &instr) noexcept
{
    return exec_rv64i_reg_imm(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<5, 0>(rhs);
    });
}

// RVI control transfer instructions

bool exec_jal(Hart &h, const Instruction &instr) noexcept
{
    h.gprs().set_reg(instr.rd, h.get_pc() + sizeof(RawInstruction));
    h.set_pc(h.get_pc() + instr.imm);
    return true;
}

bool exec_jalr(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    gprs.set_reg(instr.rd, h.get_pc() + sizeof(RawInstruction));
    h.set_pc((gprs.get_reg(instr.rs1) + instr.imm) & ~DoubleWord{1});
    return true;
}

template<std::predicate<DoubleWord, DoubleWord> F>
bool exec_cond_branch(const Instruction &instr, Hart &h, F bin_op)
noexcept(std::is_nothrow_invocable_v<F, DoubleWord, DoubleWord>)
{
    const auto &gprs = h.gprs();
    if (bin_op(gprs.get_reg(instr.rs1), gprs.get_reg(instr.rs2)))
        h.set_pc(h.get_pc() + instr.imm);
    else
        h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return true;
}

bool exec_beq(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, std::equal_to{});
}

bool exec_bne(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, std::not_equal_to{});
}

bool exec_blt(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

bool exec_bltu(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, std::less{});
}

bool exec_bge(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >= to_signed(rhs);
    });
}

bool exec_bgeu(Hart &h, const Instruction &instr) noexcept
{
    return exec_cond_branch(instr, h, std::greater_equal{});
}

// RV64I load and store instructions

template<riscv_type T>
bool exec_load(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    auto value = h.memory().load<T>(gprs.get_reg(instr.rs1) + instr.imm);
    gprs.set_reg(instr.rd, sext<kNBits<T>, DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_ld(Hart &h, const Instruction &instr) noexcept { return exec_load<DoubleWord>(h, instr); }
bool exec_lw(Hart &h, const Instruction &instr) noexcept { return exec_load<Word>(h, instr); }
bool exec_lh(Hart &h, const Instruction &instr) noexcept { return exec_load<HalfWord>(h, instr); }
bool exec_lb(Hart &h, const Instruction &instr) noexcept { return exec_load<Byte>(h, instr); }

template<typename T>
requires riscv_type<T> && (!std::is_same_v<T, DoubleWord>)
bool exec_uload(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    auto value = h.memory().load<T>(gprs.get_reg(instr.rs1) + instr.imm);
    gprs.set_reg(instr.rd, static_cast<DoubleWord>(value));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_lwu(Hart &h, const Instruction &instr) noexcept { return exec_uload<Word>(h, instr); }
bool exec_lhu(Hart &h, const Instruction &instr) noexcept { return exec_uload<HalfWord>(h, instr); }
bool exec_lbu(Hart &h, const Instruction &instr) noexcept { return exec_uload<Byte>(h, instr); }

template<riscv_type T>
bool exec_store(Hart &h, const Instruction &instr) noexcept
{
    auto &gprs = h.gprs();
    h.memory().store(gprs.get_reg(instr.rs1) + instr.imm, static_cast<T>(gprs.get_reg(instr.rs2)));
    h.set_pc(h.get_pc() + sizeof(RawInstruction));
    return false;
}

bool exec_sd(Hart &h, const Instruction &instr) noexcept { return exec_store<DoubleWord>(h, instr); }
bool exec_sw(Hart &h, const Instruction &instr) noexcept { return exec_store<Word>(h, instr); }
bool exec_sh(Hart &h, const Instruction &instr) noexcept { return exec_store<HalfWord>(h, instr); }
bool exec_sb(Hart &h, const Instruction &instr) noexcept { return exec_store<Byte>(h, instr); }

// RVI memory ordering instructions

bool exec_fence(Hart &h, const Instruction &instr) { return false; } // ignore

// RVI environment call and breakpoints

bool exec_ecall(Hart &h, [[maybe_unused]] const Instruction &instr)
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

    return true;
}

bool exec_ebreak(Hart &h, [[maybe_unused]] const Instruction &instr) noexcept
{
    h.stop();
    return true;
}

} // namespace yarvs

#include "executor_table.hpp"
