#ifndef INCLUDE_INSTRUCTION_HPP
#define INCLUDE_INSTRUCTION_HPP

#include "common.hpp"
#include "identifiers.hpp" // generated header

namespace yarvs
{

struct Instruction final
{
    using reg_index_type = Byte; // shall contain at least 5 bits
    using immediate_type = DoubleWord;

    InstrID id;
    reg_index_type rs1;
    reg_index_type rs2;
    reg_index_type rd;
    HalfWord csr;
    immediate_type imm; // sign-extended on decoding

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
};

} // namespace yarvs

#endif // INCLUDE_INSTRUCTION_HPP
