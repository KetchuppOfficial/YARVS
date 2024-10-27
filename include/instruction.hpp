#ifndef INCLUDE_INSTRUCTION_HPP
#define INCLUDE_INSTRUCTION_HPP

#include "hart.hpp"
#include "identifiers.hpp" // generated header

namespace yarvs
{

struct Instruction final
{
    using reg_index_type = uint8_t; // shall contain at least 5 bits
    using immediate_type = uint32_t; // shall contain at least 20 bits
    using callback_type = void (*)(const Instruction &, Hart &);

    InstrID id;
    reg_index_type rs1;
    reg_index_type rs2;
    reg_index_type rd;
    immediate_type imm;
    callback_type callback;
};

} // namespace yarvs

#endif // INCLUDE_INSTRUCTION_HPP
