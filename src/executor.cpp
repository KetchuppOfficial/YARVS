#include <format>
#include <functional>
#include <stdexcept>

#include <unistd.h>

#include "hart.hpp"
#include "common.hpp"
#include "instruction.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

// RVI integer register-register operations

bool Hart::exec_add(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::plus{});
}

bool Hart::exec_sub(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::minus{});
}

bool Hart::exec_and(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::bit_and{});
}

bool Hart::exec_xor(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::bit_xor{});
}

bool Hart::exec_or(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::bit_or{});
}

bool Hart::exec_sltu(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, std::less{});
}

bool Hart::exec_slt(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

// RVI64 integer register-register operations

bool Hart::exec_addw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_reg(instr, std::plus{});
}

bool Hart::exec_subw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_reg(instr, std::minus{});
}

bool Hart::exec_sll(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_srl(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_sra(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

bool Hart::exec_sllw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<4, 0>(rhs);
    });
}

bool Hart::exec_srlw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<4, 0>(rhs);
    });
}

bool Hart::exec_sraw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<4, 0>(rhs);
    });
}

// RVI integer register-immediate instructions

bool Hart::exec_addi(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, std::plus{});
}

bool Hart::exec_andi(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, std::bit_and{});
}

bool Hart::exec_ori(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, std::bit_or{});
}

bool Hart::exec_xori(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, std::bit_xor{});
}

bool Hart::exec_sltiu(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, std::less{});
}

bool Hart::exec_slti(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

bool Hart::exec_lui(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, instr.imm);
    h.pc_ += sizeof(RawInstruction);
    return false;
}

bool Hart::exec_auipc(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, h.pc_ + instr.imm);
    h.pc_ += sizeof(RawInstruction);
    return false;
}

// RV64I integer register-immediate instructions

bool Hart::exec_addiw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_imm(instr, std::plus{});
}

bool Hart::exec_slli(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_srli(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_srai(Hart &h, const Instruction &instr)
{
    return h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
}

bool Hart::exec_slliw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_srliw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
}

bool Hart::exec_sraiw(Hart &h, const Instruction &instr)
{
    return h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<5, 0>(rhs);
    });
}

// RVI control transfer instructions

bool Hart::exec_jal(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, h.pc_ + sizeof(RawInstruction));
    h.pc_ += instr.imm;
    return true;
}

bool Hart::exec_jalr(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, h.pc_ + sizeof(RawInstruction));
    h.pc_ = (h.gprs_.get_reg(instr.rs1) + instr.imm) & ~DoubleWord{1};
    return true;
}

bool Hart::exec_beq(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, std::equal_to{});
}

bool Hart::exec_bne(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, std::not_equal_to{});
}

bool Hart::exec_blt(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
}

bool Hart::exec_bltu(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, std::less{});
}

bool Hart::exec_bge(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >= to_signed(rhs);
    });
}

bool Hart::exec_bgeu(Hart &h, const Instruction &instr)
{
    return h.exec_cond_branch(instr, std::greater_equal{});
}

// RV64I load and store instructions

bool Hart::exec_ld(Hart &h, const Instruction &instr) { return h.exec_load<DoubleWord>(instr); }
bool Hart::exec_lw(Hart &h, const Instruction &instr) { return h.exec_load<Word>(instr); }
bool Hart::exec_lh(Hart &h, const Instruction &instr) { return h.exec_load<HalfWord>(instr); }
bool Hart::exec_lb(Hart &h, const Instruction &instr) { return h.exec_load<Byte>(instr); }

bool Hart::exec_lwu(Hart &h, const Instruction &instr) { return h.exec_uload<Word>(instr); }
bool Hart::exec_lhu(Hart &h, const Instruction &instr) { return h.exec_uload<HalfWord>(instr); }
bool Hart::exec_lbu(Hart &h, const Instruction &instr) { return h.exec_uload<Byte>(instr); }

bool Hart::exec_sd(Hart &h, const Instruction &instr) { return h.exec_store<DoubleWord>(instr); }
bool Hart::exec_sw(Hart &h, const Instruction &instr) { return h.exec_store<Word>(instr); }
bool Hart::exec_sh(Hart &h, const Instruction &instr) { return h.exec_store<HalfWord>(instr); }
bool Hart::exec_sb(Hart &h, const Instruction &instr) { return h.exec_store<Byte>(instr); }

// RVI memory ordering instructions

bool Hart::exec_fence(Hart &h, const Instruction &instr) { return false; } // ignore

// RVI environment call and breakpoints

bool Hart::exec_ecall(Hart &h, [[maybe_unused]] const Instruction &instr)
{
    switch (auto syscall_num = h.gprs_.get_reg(Hart::kSyscallNumReg))
    {
        case 64: // write
        {
            auto fd = h.gprs_.get_reg(Hart::kSyscallArgRegs[0]);
            auto *ptr = h.mem_.host_ptr(h.gprs_.get_reg(Hart::kSyscallArgRegs[1]));
            auto size = h.gprs_.get_reg(Hart::kSyscallArgRegs[2]);

            auto res = write(fd, reinterpret_cast<const char *>(ptr), size);

            h.gprs_.set_reg(Hart::kSyscallRetReg, res);
            h.pc_ += sizeof(RawInstruction);
            break;
        }
        case 93: // exit
            h.run_ = false;
            h.status_ = h.gprs_.get_reg(Hart::kSyscallRetReg);
            break;
        default:
            throw std::runtime_error{std::format("System call {:#x} is not supported",
                                                 syscall_num)};
    }

    return true;
}

bool Hart::exec_ebreak(Hart &h, [[maybe_unused]] const Instruction &instr)
{
    h.run_ = false;
    return true;
}

} // namespace yarvs
