#ifndef INCLUDE_EXECUTOR_HPP
#define INCLUDE_EXECUTOR_HPP

#include <array>

#include "instruction.hpp"

namespace yarvs
{

class Hart;

class Executor final
{
public:

    // return value indicates whether instruction is a basic block terminator
    using callback_type = bool(*)(Hart &, const Instruction &);

    explicit Executor() = default;

    static bool execute(Hart &h, const Instruction &instr)
    {
        return callbacks_[instr.id](h, instr);
    }

private:

    static const std::array<callback_type, InstrID::kEndID> callbacks_;
};

} // namespace yarvs

#endif // INCLUDE_EXECUTOR_HPP
