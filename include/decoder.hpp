#ifndef INCLUDE_DECODER_HPP
#define INCLUDE_DECODER_HPP

#include <array>
#include <stdexcept>
#include <unordered_map>
#include <format>

#include "common.hpp"
#include "instruction.hpp"
#include "bits_manipulation.hpp"

namespace yarvs
{

class Decoder final
{
public:

    using mask_type = RawInstruction;
    using match_type = RawInstruction;
    using decoding_func_type = Instruction (*)(RawInstruction);

    explicit Decoder() = default;

    static Instruction decode(RawInstruction raw_instr)
    {
        auto opcode = get_bits<6, 0>(raw_instr);
        mask_type mask = masks_[opcode];
        if (mask != 0)
        {
            auto it = match_map_.find(raw_instr & mask);
            if (it != match_map_.end())
                return it->second(raw_instr);
        }

        throw std::invalid_argument{std::format("unknown instruction: {:#x}", raw_instr)};
    }

private:

    static constexpr DoubleWord decode_i_imm(RawInstruction raw_instr) noexcept
    {
        return sext<12, DoubleWord>(get_bits<31, 20>(raw_instr));
    }

    static constexpr DoubleWord decode_s_imm(RawInstruction raw_instr) noexcept
    {
        return sext<12, DoubleWord>((get_bits<11, 7>(raw_instr))
                                  | (get_bits<31, 25>(raw_instr) << 5));
    }

    static constexpr DoubleWord decode_b_imm(RawInstruction raw_instr) noexcept
    {
        return sext<13, DoubleWord>((get_bits<11, 8>(raw_instr) << 1)
                                  | (get_bits<30, 25>(raw_instr) << 5)
                                  | (get_bit<7>(raw_instr) << 11)
                                  | (get_bit<31>(raw_instr) << 12));
    }

    static constexpr DoubleWord decode_u_imm(RawInstruction raw_instr) noexcept
    {
        return sext<32, DoubleWord>(mask_bits<31, 12>(raw_instr));
    }

    static constexpr DoubleWord decode_j_imm(RawInstruction raw_instr) noexcept
    {
        return sext<21, DoubleWord>((get_bits<30, 21>(raw_instr) << 1)
                                  | (get_bit<20>(raw_instr) << 11)
                                  | (mask_bits<19, 12>(raw_instr))
                                  | (get_bit<31>(raw_instr) << 20));
    }

    // both are generated from risc-v opcodes
    static const std::array<mask_type, 1 << kOpcodeBitLen> masks_;
    static const std::unordered_map<match_type, decoding_func_type> match_map_;
};

} // namespace yarvs

#endif // INCLUDE_DECODER_HPP
