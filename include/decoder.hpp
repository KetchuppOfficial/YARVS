#ifndef INCLUDE_DECODER_HPP
#define INCLUDE_DECODER_HPP

#include <array>
#include <stdexcept>
#include <unordered_map>

#include <fmt/format.h>

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

        throw std::invalid_argument{fmt::format("unknown instruction: {:#x}", raw_instr)};
    }

private:

    // both are generated from risc-v opcodes
    static const std::array<mask_type, 1 << kOpcodeBitLen> masks_;
    static const std::unordered_map<match_type, decoding_func_type> match_map_;
};

} // namespace yarvs

#endif // INCLUDE_DECODER_HPP
