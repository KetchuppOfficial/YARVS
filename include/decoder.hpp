#ifndef INCLUDE_DECODER_HPP
#define INCLUDE_DECODER_HPP

#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "common.hpp"
#include "instruction.hpp"

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
        for (auto mask : masks_)
        {
            auto it = match_map_.find(raw_instr & mask);
            if (it != match_map_.end())
                return it->second(raw_instr);
        }

        throw std::invalid_argument{fmt::format("unknown instruction: {:#x}", raw_instr)};
    }

private:

    // both are generated from risc-v opcodes
    static const std::vector<mask_type> masks_;
    static const std::unordered_map<match_type, decoding_func_type> match_map_;
};

} // namespace yarvs

#endif // INCLUDE_DECODER_HPP
