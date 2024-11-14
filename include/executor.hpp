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

    using callback_type = void(*)(Hart &, const Instruction &);

    explicit Executor() = default;

    static void execute(Hart &h, const Instruction &instr) { callbacks_[instr.id](h, instr); }

private:

    static const std::array<callback_type, InstrID::kEndID> callbacks_;
};

} // namespace yarvs

#endif // INCLUDE_EXECUTOR_HPP
