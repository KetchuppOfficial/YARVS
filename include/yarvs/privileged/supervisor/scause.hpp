#ifndef INCLUDE_PRIVILEGED_SUPERVISOR_SCAUSE_HPP
#define INCLUDE_PRIVILEGED_SUPERVISOR_SCAUSE_HPP

#include <utility>

#include "yarvs/common.hpp"
#include "yarvs/bits_manipulation.hpp"

namespace yarvs
{

// supervisor cause register
class SCause final
{
public:

    enum Exception : DoubleWord
    {
        kInstrAddrMisaligned = 0,
        kInstrAccessFault = 1,
        kIllegalInstruction = 2,
        kBreakpoint = 3,
        kLoadAddrMisaligned = 4,
        kLoadAccessFault = 5,
        kStoreAMOAddrMisaligned = 6,
        kStoreAMOAccessFault = 7,
        kEnvCallFromUMode = 8,
        kEnvCallFromSMode = 9,
        // 10-11; reserved
        kInstrPageFault = 12,
        kLoadPageFault = 13,
        // 14: reserved
        kStoreAMOPageFault = 15,
        // 16-17: reserved
        kSoftwareCheck = 18,
        kHardwareError = 19
        // 20-23: reserved
        // 24-31: designated for custom use
        // 32-47: reserved
        // 48-63: designated for custom use
        // >= 64: reserved
    };

    enum Interrupt : DoubleWord
    {
        // 0: reserved
        kSupervisorSoftwareInt = 1,
        // 2-4: reserved
        kSupervisorTimerInt = 5,
        // 6-8: reserved
        kSupervisorExternalInt = 9,
        // 10-12: reserved
        kCounterOverflowInt = 13
        // 14-15: reserved
        // >= 16: designated for platform use
    };

    constexpr SCause() noexcept : value_{DoubleWord{1} << 63} {} // initialize with reserved value
    constexpr SCause(DoubleWord value) noexcept : value_{value} {}

    constexpr operator DoubleWord() noexcept { return value_; }

    constexpr std::pair<DoubleWord, bool> get_cause() const noexcept
    {
        return std::pair{mask_bits<62, 0>(value_), static_cast<bool>(mask_bit<63>(value_))};
    }

    constexpr void set_interrupt(Interrupt i) noexcept { value_ = set_bit<63>(+i, true); }
    constexpr void set_exception(Exception e) noexcept { value_ = set_bit<63>(+e, false); }

    constexpr const char *what() const noexcept
    {
        const auto [value, is_int] = get_cause();

        if (is_int)
        {
            switch (value)
            {
                case kSupervisorSoftwareInt:
                    return "supervisor sofware interrupt";
                case kSupervisorTimerInt:
                    return "supervisor timer interrupt";
                case kSupervisorExternalInt:
                    return "supervisor external interrupt";
                case kCounterOverflowInt:
                    return "counter-overflow interrupt";
                default:
                    return nullptr;
            }
        }

        switch (value)
        {
            case kInstrAddrMisaligned:
                return "instruction address misaligned";
            case kInstrAccessFault:
                return "instruction access fault";
            case kIllegalInstruction:
                return "illegal instruction";
            case kBreakpoint:
                return "breakpoint";
            case kLoadAddrMisaligned:
                return "load address misaligned";
            case kLoadAccessFault:
                return "load access fault";
            case kStoreAMOAddrMisaligned:
                return "store/AMO address misaligned";
            case kStoreAMOAccessFault:
                return "store/AMO access fault";
            case kEnvCallFromUMode:
                return "environment call from U-mode";
            case kEnvCallFromSMode:
                return "environment call from S-mode";
            case kInstrPageFault:
                return "instruction page fault";
            case kLoadPageFault:
                return "load page fault";
            case kStoreAMOPageFault:
                return "store/AMO page fault";
            case kSoftwareCheck:
                return "software check";
            case kHardwareError:
                return "hardware error";
            default:
                return nullptr;
        }
    }

private:

    DoubleWord value_;
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_SUPERVISOR_SCAUSE_HPP
