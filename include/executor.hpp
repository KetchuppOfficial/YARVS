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
    using callback_type = void(*)(const Instruction &, Hart &);

    explicit Executor() = default;

    static void execute(const Instruction &instr, Hart &h) { callbacks_[instr.id](instr, h); }

private:

    static const std::array<callback_type, InstrID::kEndID> callbacks_;
};

} // namespace yarvs

#endif // INCLUDE_EXECUTOR_HPP
