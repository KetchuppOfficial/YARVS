#ifndef INCLUDE_INSTRUCTION_HPP
#define INCLUDE_INSTRUCTION_HPP

#include <string>

#include "yarvs/common.hpp"
#include "yarvs/identifiers.hpp" // generated header

namespace yarvs
{

struct Instruction final
{
    using gpr_index_type = Byte; // shall contain at least 5 bits
    using immediate_type = DoubleWord;

    RawInstruction raw;
    InstrID id;
    gpr_index_type rs1;
    gpr_index_type rs2;
    gpr_index_type rd;

    /*
     * 1. sign-extended on decoding
     * 2. used as a subscript into CSR regfile
     */
    immediate_type imm;

    bool is_terminator() const noexcept
    {
        switch (id)
        {
            case InstrID::kBEQ:
            case InstrID::kBGE:
            case InstrID::kBGEU:
            case InstrID::kBLT:
            case InstrID::kBLTU:
            case InstrID::kBNE:
            case InstrID::kEBREAK:
            case InstrID::kECALL:
            case InstrID::kJAL:
            case InstrID::kJALR:
            case InstrID::kMRET:
            case InstrID::kSRET:
                return true;
            default:
                return false;
        }
    }

    std::string disassemble() const;
};

} // namespace yarvs

#endif // INCLUDE_INSTRUCTION_HPP
