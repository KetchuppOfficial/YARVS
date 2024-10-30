#ifndef INCLUDE_INSTRUCTION_HPP
#define INCLUDE_INSTRUCTION_HPP

#include "common.hpp"
#include "hart.hpp"
#include "identifiers.hpp" // generated header

namespace yarvs
{

struct Instruction final
{
    using reg_index_type = Byte; // shall contain at least 5 bits
    using immediate_type = DoubleWord;
    using callback_type = void (*)(const Instruction &, Hart &);

    InstrID id;
    reg_index_type rs1;
    reg_index_type rs2;
    reg_index_type rd;
    immediate_type imm; // sign-extended on decoding
    callback_type callback;
};

} // namespace yarvs

#endif // INCLUDE_INSTRUCTION_HPP
