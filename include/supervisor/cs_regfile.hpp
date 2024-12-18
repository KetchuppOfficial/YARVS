#ifndef INCLUDE_CSR_HPP
#define INCLUDE_CSR_HPP

#include "supervisor/satp.hpp"
#include "supervisor/scause.hpp"
#include "supervisor/stvec.hpp"

namespace yarvs
{

struct CSRegfile final
{
    STVec stvec;
    SCause scause;
    SATP satp;
};

} // namespace yarvs

#endif // INCLUDE_CSR_HPP
