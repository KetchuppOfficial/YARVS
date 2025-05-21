#ifndef INCLUDE_DECODER_HPP
#define INCLUDE_DECODER_HPP

#include "yarvs/bits_manipulation.hpp"
#include "yarvs/common.hpp"
#include "yarvs/instruction.hpp"

namespace yarvs
{

class Decoder final
{
public:

    using mask_type = RawInstruction;
    using match_type = RawInstruction;
    using decoding_func_type = Instruction (*)(RawInstruction);

    explicit Decoder() = default;

    static Instruction decode(RawInstruction raw_instr); // generated from risc-v opcodes

private:

    // generated from risc-v opcodes
    static decoding_func_type get_decoder(match_type match) noexcept;

    static constexpr DoubleWord decode_i_imm(RawInstruction raw_instr) noexcept
    {
        return sext<12, DoubleWord>(get_bits<31, 20>(raw_instr));
    }

    static constexpr DoubleWord decode_s_imm(RawInstruction raw_instr) noexcept
    {
        return sext<12, DoubleWord>((get_bits<11, 7>(raw_instr))
                                  | (mask_bits<31, 25>(raw_instr) >> 20));
    }

    static constexpr DoubleWord decode_b_imm(RawInstruction raw_instr) noexcept
    {
        return sext<13, DoubleWord>((mask_bits<11, 8>(raw_instr) >> 7)
                                  | (mask_bits<30, 25>(raw_instr) >> 20)
                                  | (mask_bit<7>(raw_instr) << 4)
                                  | (mask_bit<31>(raw_instr) >> 19));
    }

    static constexpr DoubleWord decode_u_imm(RawInstruction raw_instr) noexcept
    {
        return sext<32, DoubleWord>(mask_bits<31, 12>(raw_instr));
    }

    static constexpr DoubleWord decode_j_imm(RawInstruction raw_instr) noexcept
    {
        return sext<21, DoubleWord>((mask_bits<30, 21>(raw_instr) >> 20)
                                  | (mask_bit<20>(raw_instr) >> 9)
                                  | (mask_bits<19, 12>(raw_instr))
                                  | (mask_bit<31>(raw_instr) >> 11));
    }
};

} // namespace yarvs

#endif // INCLUDE_DECODER_HPP
