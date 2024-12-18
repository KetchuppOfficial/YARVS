#ifndef INCLUDE_MEMORY_MEMORY_HPP
#define INCLUDE_MEMORY_MEMORY_HPP

#include <cstddef>
#include <expected>
#include <iterator>
#include <type_traits>
#include <utility>

#include "common.hpp"
#include "bits_manipulation.hpp"

#include "memory/mmap_wrapper.hpp"
#include "memory/virtual_address.hpp"
#include "memory/pte.hpp"

#include "privileged/cs_regfile.hpp"

namespace yarvs
{

class Memory final
{
public:

    static constexpr DoubleWord kPageBits = 12;
    static constexpr DoubleWord kPageSize = 1 << kPageBits;
    static constexpr std::size_t kPhysMemAmount = 4 * (std::size_t{1} << 30); // 4GB

    explicit Memory(CSRegfile &csrs, const PrivilegeLevel &priv_mode)
        : physical_mem_{kPhysMemAmount, MMapWrapper::kRead | MMapWrapper::kWrite},
          csrs_{csrs}, priv_level_{priv_mode} {}

    enum FaultType
    {
        kPageFault
    };

    template<riscv_type T>
    std::expected<T, MCause::Exception> load(DoubleWord va)
    {
        if (!csrs_.is_satp_active(priv_level_))
            return pm_load<T>(va);
        auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
        if (!maybe_pa.has_value()) [[unlikely]]
            return std::unexpected{MCause::Exception::kLoadPageFault};
        return pm_load<T>(*maybe_pa);
    }

    template<riscv_type T>
    std::expected<void, MCause::Exception> store(DoubleWord va, T value)
    {
        if (!csrs_.is_satp_active(priv_level_))
            pm_store(va, value);
        else
        {
            auto maybe_pa = translate_address<MemoryAccessType::kWrite>(va);
            if (!maybe_pa.has_value()) [[unlikely]]
                return std::unexpected{MCause::Exception::kStoreAMOPageFault};
            pm_store(*maybe_pa, value);
        }
        return {};
    }

    template<std::input_iterator It>
    void store(DoubleWord va, It first, It last)
    {
        using value_type = std::remove_const_t<typename std::iterator_traits<It>::value_type>;
        for (std::size_t i = 0; first != last; ++first, ++i)
            store(va + i * sizeof(value_type), *first);
    }

    std::expected<RawInstruction, MCause::Exception> fetch(DoubleWord va)
    {
        if (!csrs_.is_satp_active(priv_level_))
            return pm_load<RawInstruction>(va);
        auto maybe_pa = translate_address<MemoryAccessType::kExecute>(va);
        if (!maybe_pa.has_value()) [[unlikely]]
            return std::unexpected{MCause::Exception::kInstrPageFault};
        return pm_load<RawInstruction>(*maybe_pa);
    }

    std::expected<const Byte *, MCause::Exception> host_ptr(DoubleWord va)
    {
        if (!csrs_.is_satp_active(priv_level_))
            return &physical_mem_[va];
        auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
        if (!maybe_pa.has_value()) [[unlikely]]
            return std::unexpected{MCause::Exception::kLoadPageFault};
        return &physical_mem_[*maybe_pa];
    }

private:

    enum MemoryAccessType
    {
        kRead,
        kWrite,
        kExecute
    };

    template<MemoryAccessType kAccessKind>
    std::expected<DoubleWord, FaultType> translate_address(DoubleWord va)
    {
        switch (auto mode = csrs_.get_satp().get_mode())
        {
            case SATP::Mode::kBare:
                return va;
            case SATP::Mode::kSv39:
                if (va != sext<39, DoubleWord>(va))
                    return std::unexpected{FaultType::kPageFault};
                return translate<kAccessKind, 3>(va);
            case SATP::Mode::kSv48:
                if (va != sext<48, DoubleWord>(va))
                    return std::unexpected{FaultType::kPageFault};
                return translate<kAccessKind, 4>(va);
            case SATP::Mode::kSv57:
                if (va != sext<57, DoubleWord>(va))
                    return std::unexpected{FaultType::kPageFault};
                return translate<kAccessKind, 5>(va);
            default: [[unlikely]]
                std::unreachable();
        }
    }

    template<MemoryAccessType kAccessKind, Byte kLevels>
    std::expected<DoubleWord, FaultType> translate(VirtualAddress va)
    {
        const SStatus sstatus = csrs_.get_sstatus();
        DoubleWord a = csrs_.get_satp().get_ppn() * kPageSize;

        PTE pte;
        auto i = kLevels - 1;
        for (;;)
        {
            const DoubleWord pa = a + va.get_vpn(i) * sizeof(PTE);
            pte = pm_load<DoubleWord>(pa);

            if (!pte.get_V() || pte.is_rwx_reserved() || pte.uses_reserved())
                return std::unexpected{FaultType::kPageFault};

            if (pte.is_pointer_to_next_level_pte())
            {
                if (i == 0)
                    return std::unexpected{FaultType::kPageFault};
                --i;
                a = pte.get_ppn() * kPageSize;
                continue;
            }

            if constexpr (kAccessKind == MemoryAccessType::kRead)
            {
                if (!pte.get_R() || (!sstatus.get_mxr() && pte.get_E()))
                    return std::unexpected{FaultType::kPageFault};
            }
            else if constexpr (kAccessKind == MemoryAccessType::kWrite)
            {
                if (!pte.get_W())
                    return std::unexpected{FaultType::kPageFault};
            }
            else
            {
                if (!pte.get_E())
                    return std::unexpected{FaultType::kPageFault};
            }

            if (priv_level_ == PrivilegeLevel::kSupervisor && !sstatus.get_sum() && pte.get_U())
                return std::unexpected{FaultType::kPageFault};

            if (!pte.get_A() || (kAccessKind == MemoryAccessType::kWrite && !pte.get_D()))
            {
                if (pte != pm_load<DoubleWord>(pa))
                    continue;

                pte.set_A(true);
                if constexpr (kAccessKind == MemoryAccessType::kWrite)
                    pte.set_D(true);
            }

            pm_store(pa, +pte);
            return (pte.get_ppn() << kPageBits) | va.get_page_offset();
        }
    }

    template<riscv_type T>
    T pm_load(DoubleWord pa) const { return *reinterpret_cast<const T *>(&physical_mem_[pa]); }

    template<riscv_type T>
    void pm_store(DoubleWord pa, T value) { *reinterpret_cast<T*>(&physical_mem_[pa]) = value; }

    MMapWrapper physical_mem_;
    CSRegfile &csrs_;
    const PrivilegeLevel &priv_level_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_MEMORY_HPP
