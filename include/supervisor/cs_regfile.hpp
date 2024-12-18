#ifndef INCLUDE_CSR_HPP
#define INCLUDE_CSR_HPP

#include "common.hpp"

#include "supervisor/sstatus.hpp"
#include "supervisor/satp.hpp"
#include "supervisor/scause.hpp"
#include "supervisor/stvec.hpp"

namespace yarvs
{

struct CSRegfile final
{
    SStatus sstatus;

    STVec stvec;
    DoubleWord sscratch = 0; // supervisor scratch register
    DoubleWord sepc = 0; // supervisor exception program counter
    SCause scause;
    DoubleWord stval = 0; // supervisor trap value
    SATP satp;
};

} // namespace yarvs

#endif // INCLUDE_CSR_HPP
