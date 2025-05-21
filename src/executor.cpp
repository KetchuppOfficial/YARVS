#include <functional>
#include <stdexcept>

#include <unistd.h>

#include <fmt/format.h>

#include "yarvs/bits_manipulation.hpp"
#include "yarvs/common.hpp"
#include "yarvs/hart.hpp"
#include "yarvs/instruction.hpp"

#include "yarvs/privileged/cs_regfile.hpp"

namespace yarvs
{

// RVI integer register-register operations

bool Hart::exec_add(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::plus{});
    return true;
}

bool Hart::exec_sub(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::minus{});
    return true;
}

bool Hart::exec_and(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::bit_and{});
    return true;
}

bool Hart::exec_xor(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::bit_xor{});
    return true;
}

bool Hart::exec_or(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::bit_or{});
    return true;
}

bool Hart::exec_sltu(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, std::less{});
    return true;
}

bool Hart::exec_slt(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
    return true;
}

// RVI64 integer register-register operations

bool Hart::exec_addw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_reg(instr, std::plus{});
    return true;
}

bool Hart::exec_subw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_reg(instr, std::minus{});
    return true;
}

bool Hart::exec_sll(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_srl(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_sra(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
    return true;
}

bool Hart::exec_sllw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<4, 0>(rhs);
    });
    return true;
}

bool Hart::exec_srlw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<4, 0>(rhs);
    });
    return true;
}

bool Hart::exec_sraw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_reg(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<4, 0>(rhs);
    });
    return true;
}

// RVI integer register-immediate instructions

bool Hart::exec_addi(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, std::plus{});
    return true;
}

bool Hart::exec_andi(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, std::bit_and{});
    return true;
}

bool Hart::exec_ori(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, std::bit_or{});
    return true;
}

bool Hart::exec_xori(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, std::bit_xor{});
    return true;
}

bool Hart::exec_sltiu(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, std::less{});
    return true;
}

bool Hart::exec_slti(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
    return true;
}

bool Hart::exec_lui(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, instr.imm);
    h.pc_ += sizeof(RawInstruction);
    return true;
}

bool Hart::exec_auipc(Hart &h, const Instruction &instr)
{
    h.gprs_.set_reg(instr.rd, h.pc_ + instr.imm);
    h.pc_ += sizeof(RawInstruction);
    return true;
}

// RV64I integer register-immediate instructions

bool Hart::exec_addiw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_imm(instr, std::plus{});
    return true;
}

bool Hart::exec_slli(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_srli(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_srai(Hart &h, const Instruction &instr)
{
    h.exec_rvi_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_unsigned(to_signed(lhs) >> mask_bits<5, 0>(rhs));
    });
    return true;
}

bool Hart::exec_slliw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs << mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_srliw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return lhs >> mask_bits<5, 0>(rhs);
    });
    return true;
}

bool Hart::exec_sraiw(Hart &h, const Instruction &instr)
{
    h.exec_rv64i_reg_imm(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >> mask_bits<5, 0>(rhs);
    });
    return true;
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
    h.exec_cond_branch(instr, std::equal_to{});
    return true;
}

bool Hart::exec_bne(Hart &h, const Instruction &instr)
{
    h.exec_cond_branch(instr, std::not_equal_to{});
    return true;
}

bool Hart::exec_blt(Hart &h, const Instruction &instr)
{
    h.exec_cond_branch(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) < to_signed(rhs);
    });
    return true;
}

bool Hart::exec_bltu(Hart &h, const Instruction &instr)
{
    h.exec_cond_branch(instr, std::less{});
    return true;
}

bool Hart::exec_bge(Hart &h, const Instruction &instr)
{
    h.exec_cond_branch(instr, [](auto lhs, auto rhs)
    {
        return to_signed(lhs) >= to_signed(rhs);
    });
    return true;
}

bool Hart::exec_bgeu(Hart &h, const Instruction &instr)
{
    h.exec_cond_branch(instr, std::greater_equal{});
    return true;
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

bool Hart::exec_fence([[maybe_unused]] Hart &h, [[maybe_unused]] const Instruction &instr)
{
    return true; // ignore
}

// RVI environment call and breakpoints

bool Hart::exec_ecall(Hart &h, [[maybe_unused]] const Instruction &instr)
{
    switch (auto syscall_num = h.gprs_.get_reg(Hart::kSyscallNumReg))
    {
        case 64: // write
        {
            auto fd = h.gprs_.get_reg(Hart::kSyscallArgRegs[0]);
            const auto va = h.gprs_.get_reg(Hart::kSyscallArgRegs[1]);
            auto maybe_ptr = h.mem_.host_ptr(va);
            if (!maybe_ptr.has_value()) [[unlikely]]
            {
                h.raise_exception(maybe_ptr.error(), va);
                return false;
            }

            auto size = h.gprs_.get_reg(Hart::kSyscallArgRegs[2]);

            auto res = write(fd, reinterpret_cast<const char *>(*maybe_ptr), size);

            h.gprs_.set_reg(Hart::kSyscallRetReg, res);
            h.pc_ += sizeof(RawInstruction);
            break;
        }
        case 93: // exit
            h.run_ = false;
            h.status_ = h.gprs_.get_reg(Hart::kSyscallRetReg);
            break;
        default:
            throw std::runtime_error{fmt::format("System call {} at pc {:#x} is not supported",
                                                 syscall_num, h.pc_)};
    }

    return true;
}

bool Hart::exec_ebreak(Hart &h, [[maybe_unused]] const Instruction &instr)
{
    h.run_ = false;
    return true;
}

// Zicsr extension

bool Hart::exec_csrrw(Hart &h, const Instruction &instr)
{
    return h.exec_csrrw_csrrwi(instr, [&gprs = h.gprs_](const Instruction &instr)
    {
        return gprs.get_reg(instr.rs1);
    });
}

bool Hart::exec_csrrwi(Hart &h, const Instruction &instr)
{
    return h.exec_csrrw_csrrwi(instr, &Instruction::rs1);
}

bool Hart::exec_csrrs(Hart &h, const Instruction &instr)
{
    return h.exec_csrrs_csrrc(instr, std::bit_or{}, [&gprs = h.gprs_](const Instruction &instr)
    {
        return gprs.get_reg(instr.rs1);
    });
}

bool Hart::exec_csrrc(Hart &h, const Instruction &instr)
{
    return h.exec_csrrs_csrrc(instr, [](auto lhs, auto rhs){ return lhs & ~rhs; },
                              [&gprs = h.gprs_](const Instruction &instr)
    {
        return gprs.get_reg(instr.rs1);
    });
}

bool Hart::exec_csrrsi(Hart &h, const Instruction &instr)
{
    return h.exec_csrrs_csrrc(instr, std::bit_or{}, &Instruction::rs1);
}

bool Hart::exec_csrrci(Hart &h, const Instruction &instr)
{
    return h.exec_csrrs_csrrc(instr, [](auto lhs, auto rhs){ return lhs & ~rhs; },
                              &Instruction::rs1);
}

// System instructions

bool Hart::exec_sret(Hart &h, const Instruction &instr)
{
    SStatus sstatus = h.csrs_.get_sstatus();
    auto old_priv_mode = static_cast<PrivilegeLevel>(sstatus.get_spp());
    h.priv_level_ = old_priv_mode;
    sstatus.set_sie(sstatus.get_spie());
    sstatus.set_spie(true);
    sstatus.set_spp(PrivilegeLevel::kUser);
    h.csrs_.set_sstatus(sstatus);

    if (old_priv_mode != PrivilegeLevel::kMachine)
    {
        MStatus mstatus = h.csrs_.get_mstatus();
        mstatus.set_mprv(false);
        h.csrs_.set_mstatus(mstatus);
    }

    h.pc_ = h.csrs_.get_sepc();

    return true;
}

bool Hart::exec_mret(Hart &h, const Instruction &instr)
{
    MStatus mstatus = h.csrs_.get_mstatus();
    h.priv_level_ = static_cast<PrivilegeLevel>(mstatus.get_mpp());
    mstatus.set_mie(mstatus.get_mpie());
    mstatus.set_mpie(true);
    mstatus.set_mpp(PrivilegeLevel::kUser);
    h.csrs_.set_mstatus(mstatus);

    h.pc_ = h.csrs_.get_mepc();

    return true;
}

bool Hart::exec_wfi(Hart &h, const Instruction &instr)
{
    throw std::runtime_error{"WFI instruction is not implemented"};
}

bool Hart::exec_sfence_vma(Hart &h, const Instruction &instr)
{
    throw std::runtime_error{"SFENCE.VMA is not implemented"};
}

} // namespace yarvs
