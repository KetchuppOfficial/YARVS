#ifndef INCLUDE_PRIVILEGED_CS_REGFILE_HPP
#define INCLUDE_PRIVILEGED_CS_REGFILE_HPP

#include <array>
#include <cassert>

#include "common.hpp"

#include "privileged/xtvec.hpp"

#include "privileged/supervisor/sstatus.hpp"
#include "privileged/supervisor/satp.hpp"
#include "privileged/supervisor/scause.hpp"

#include "privileged/machine/mstatus.hpp"
#include "privileged/machine/mcause.hpp"

namespace yarvs
{

class CSRegfile final
{
public:

    enum CSR : HalfWord
    {
        kSStatus = 0x100,
        kSTVec = 0x105,
        kSScratch = 0x140,
        kSEPC = 0x141,
        kSCause = 0x142,
        kSTVal = 0x143,
        kSATP = 0x180,

        kMStatus = 0x300,
        kMEDeleg = 0x302,
        kMTVec = 0x305,
        kMScratch = 0x340,
        kMEPC = 0x341,
        kMCause = 0x342,
        kMTVal = 0x343
    };

    static constexpr std::size_t kNRegs = 4096;

    CSRegfile() = default;

    CSRegfile(const CSRegfile &rhs) = delete;
    CSRegfile &operator=(const CSRegfile &rhs) = delete;

    CSRegfile(CSRegfile &&rhs) = delete;
    CSRegfile &operator=(CSRegfile &&rhs) = delete;

    DoubleWord get_reg(std::size_t i) const noexcept
    {
        assert(i < kNRegs);
        return csrs_[i];
    }

    void set_reg(std::size_t i, DoubleWord csr) noexcept
    {
        assert(i < kNRegs);
        csrs_[i] = csr;
    }

    // supervisor status register
    SStatus get_sstatus() const noexcept { return get_reg(kSStatus); }
    void set_sstatus(DoubleWord v) noexcept
    {
        set_reg(kSStatus, v);
        set_reg(kMStatus, get_reg(kMStatus) | v);
    }

    // supervisor trap handler base address
    XTVec get_stvec() const noexcept { return get_reg(kSTVec); }
    void set_stvec(DoubleWord v) noexcept { return set_reg(kSTVec, v); }

    // scratch register for supervisor trap handlers
    DoubleWord get_sscratch() const noexcept { return get_reg(kSScratch); }
    void set_sscratch(DoubleWord v) noexcept { set_reg(kSScratch, v); }

    // supervisor exception program counter
    DoubleWord get_sepc() const noexcept { return get_reg(kSEPC); }
    void set_sepc(DoubleWord v) noexcept { set_reg(kSEPC, v); }

    // supervisor trap cause
    SCause get_scause() const noexcept { return get_reg(kSCause); }
    void set_scause(DoubleWord v) noexcept { set_reg(kSCause, v); }

    // supervisor trap value
    DoubleWord get_stval() const noexcept { return get_reg(kSTVal); }
    void set_stval(DoubleWord v) noexcept { set_reg(kSTVal, v); }

    // supervisor protection and translation
    SATP get_satp() const noexcept { return get_reg(kSATP); }
    void set_satp(DoubleWord v) noexcept { set_reg(kSATP, v); }
    bool is_satp_active(PrivilegeLevel current_level) const noexcept
    {
        const auto mstatus = get_mstatus();
        const auto effective_priv_mode = mstatus.get_mprv() ? mstatus.get_mpp() : current_level;
        return effective_priv_mode != PrivilegeLevel::kMachine;
    }

    // machine status register
    MStatus get_mstatus() const noexcept { return get_reg(kMStatus); }
    void set_mstatus(DoubleWord v) noexcept
    {
        set_reg(kMStatus, v);
        set_reg(kSStatus, get_reg(kSStatus) | v);
    }

    DoubleWord get_medeleg() const noexcept { return get_reg(kMEDeleg); }
    void set_medeleg(DoubleWord v) noexcept { set_reg(kMEDeleg, v); }

    // machine trap handler base address
    XTVec get_mtvec() const noexcept { return get_reg(kMTVec); }
    void set_mtvec(DoubleWord v) noexcept { set_reg(kMTVec, v); }

    // scratch register for machine trap handlers
    DoubleWord get_mscratch() const noexcept { return get_reg(kMScratch); }
    void set_mscratch(DoubleWord v) noexcept { set_reg(kMScratch, v); }

    // machine exception program counter
    DoubleWord get_mepc() const noexcept { return get_reg(kMEPC); }
    void set_mepc(DoubleWord v) noexcept { set_reg(kMEPC, v); }

    // supervisor trap cause
    MCause get_mcause() const noexcept { return get_reg(kMCause); }
    void set_mcause(DoubleWord v) noexcept { set_reg(kMCause, v); }

    // supervisor trap value
    DoubleWord get_mtval() const noexcept { return get_reg(kMTVal); }
    void set_mtval(DoubleWord v) noexcept { set_reg(kMTVal, v); }

private:

    std::array<DoubleWord, kNRegs> csrs_{};
};

} // namespace yarvs

#endif // INCLUDE_PRIVILEGED_CS_REGFILE_HPP
