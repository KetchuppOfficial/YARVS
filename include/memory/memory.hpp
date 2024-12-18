#ifndef INCLUDE_MEMORY_MEMORY_HPP
#define INCLUDE_MEMORY_MEMORY_HPP

#include <stdexcept>
#include <cstddef>
#include <expected>
#include <iterator>
#include <type_traits>
#include <utility>

#include "common.hpp"
#include "bits_manipulation.hpp"

#include "supervisor/cs_regfile.hpp"

#include "memory/mmap_wrapper.hpp"
#include "memory/virtual_address.hpp"
#include "memory/pte.hpp"

namespace yarvs
{

class Memory final
{
public:

    static constexpr DoubleWord kPageBits = 12;
    static constexpr DoubleWord kPageSize = 1 << kPageBits;
    static constexpr std::size_t kPhysMemAmount = 4 * (std::size_t{1} << 30); // 4GB

    explicit Memory(const CSRegfile &csrs)
        : physical_mem_{kPhysMemAmount, MMapWrapper::kRead | MMapWrapper::kWrite}, csrs_{csrs} {}

    enum FaultType
    {
        kPageFault
    };

    template<riscv_type T>
    T load(DoubleWord va)
    {
        auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
        if (maybe_pa.has_value()) [[ likely ]]
            return pm_load<T>(*maybe_pa);
        else
            throw std::runtime_error{"page fault"}; // temporary
    }

    template<riscv_type T>
    void store(DoubleWord va, T value)
    {
        auto maybe_pa = translate_address<MemoryAccessType::kWrite>(va);
        if (maybe_pa.has_value()) [[ likely ]]
            pm_store(*maybe_pa, value);
        else
            throw std::runtime_error{"page fault"}; // temporary
    }

    template<std::input_iterator It>
    void store(DoubleWord va, It first, It last)
    {
        using value_type = std::remove_const_t<typename std::iterator_traits<It>::value_type>;
        for (std::size_t i = 0; first != last; ++first, ++i)
            store(va + i * sizeof(value_type), *first);
    }

    RawInstruction fetch(DoubleWord va)
    {
        auto maybe_pa = translate_address<MemoryAccessType::kExecute>(va);
        if (maybe_pa.has_value()) [[ likely ]]
            return pm_load<RawInstruction>(*maybe_pa);
        else
            throw std::runtime_error{"page fault"}; // temporary
    }

    const Byte *host_ptr(DoubleWord va)
    {
        auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
        if (maybe_pa.has_value()) [[likely]]
            return &physical_mem_[*maybe_pa];
        else
            throw std::runtime_error{"page fault"}; // temporary
    }

private:

    friend class Hart;

    enum MemoryAccessType
    {
        kRead,
        kWrite,
        kExecute
    };

    template<MemoryAccessType kAccessKind>
    std::expected<DoubleWord, FaultType> translate_address(DoubleWord va)
    {
        switch (auto mode = csrs_.satp.get_mode())
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
        DoubleWord a = csrs_.satp.get_ppn() * kPageSize;

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

            // I don't use <if constexpr>, because the optimizer is expected to remove certain
            // control-flow branches.
            if (kAccessKind == MemoryAccessType::kRead    && !pte.get_R() ||
                kAccessKind == MemoryAccessType::kWrite   && !pte.get_W() ||
                kAccessKind == MemoryAccessType::kExecute && !pte.get_E())
            {
                return std::unexpected{FaultType::kPageFault};
            }

            if (!pte.get_A() || (kAccessKind == MemoryAccessType::kWrite && !pte.get_D()))
            {
                if (pte != PTE{pm_load<DoubleWord>(pa)})
                    continue;

                pte.set_A(true);
                if constexpr (kAccessKind == MemoryAccessType::kWrite)
                    pte.set_D(true);
            }

            pm_store(pa, static_cast<DoubleWord>(pte));
            return (pte.get_ppn() << kPageBits) | va.get_page_offset();
        }
    }

    template<riscv_type T>
    T pm_load(DoubleWord pa) const { return *reinterpret_cast<const T *>(&physical_mem_[pa]); }

    template<riscv_type T>
    void pm_store(DoubleWord pa, T value) { *reinterpret_cast<T*>(&physical_mem_[pa]) = value; }

    MMapWrapper physical_mem_;
    const CSRegfile &csrs_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_MEMORY_HPP
