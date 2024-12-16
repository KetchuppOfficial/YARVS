#ifndef INCLUDE_MEMORY_MEMORY_HPP
#define INCLUDE_MEMORY_MEMORY_HPP

#include <stdexcept>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <type_traits>

#include "common.hpp"
#include "bits_manipulation.hpp"
#include "csr.hpp"

#include "memory/mmap_wrapper.hpp"
#include "memory/virtual_address.hpp"
#include "memory/pte.hpp"

namespace yarvs
{

class Memory final
{
public:

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
        if (maybe_pa.has_value())
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

    enum MemoryAccessType
    {
        kRead,
        kWrite,
        kExecute
    };

    static constexpr DoubleWord kPageBits = 12;
    static constexpr DoubleWord kPageSize = 1 << kPageBits;
    static constexpr std::size_t kPhysMemAmount = 4 * (std::size_t{1} << 30); // 4GB

    template<MemoryAccessType kAccessKind>
    std::expected<DoubleWord, FaultType> translate_address(DoubleWord va)
    {
        switch (auto mode = csrs_.satp.get_mode())
        {
            case SATP::Mode::kBare:
                return va;
            case SATP::Mode::kSv39:
                return translate<kAccessKind, 3>(va);
            case SATP::Mode::kSv48:
                return translate<kAccessKind, 4>(va);
            case SATP::Mode::kSv57:
                return translate<kAccessKind, 5>(va);
            default: [[unlikely]]
                throw std::runtime_error{
                    std::format("Translation mode {} is not supported", Byte{mode})};
        }
    }

    template<MemoryAccessType kAccessKind, Byte kLevels>
    std::expected<DoubleWord, FaultType> translate(DoubleWord raw_va)
    {
        if (mask_bits<38, 0>(raw_va) != sext<39, DoubleWord>(raw_va))
            return std::unexpected{FaultType::kPageFault};

        DoubleWord a = csrs_.satp.get_ppn() * kPageSize;
        const VirtualAddress va{raw_va};

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

            // I don't use if constexpr, because the optimizer is expected to remove certain
            // control-flow branches.
            if (kAccessKind == MemoryAccessType::kRead    && !pte.get_R() ||
                kAccessKind == MemoryAccessType::kWrite   && !pte.get_W() ||
                kAccessKind == MemoryAccessType::kExecute && !pte.get_E())
            {
                return std::unexpected{FaultType::kPageFault};
            }

            if (!pte.get_A() || (kAccessKind == MemoryAccessType::kWrite && !pte.get_D()))
            {
                PTE another_pte = pm_load<DoubleWord>(a + va.get_vpn(i) * sizeof(PTE));
                if (another_pte == pte)
                {
                    pte.set_A();
                    if constexpr (kAccessKind == MemoryAccessType::kWrite)
                        pte.set_D();
                    pm_store(pa, static_cast<DoubleWord>(pte));
                    break;
                }
            }
        }

        return (pte.get_ppn() << kPageBits) | va.get_page_offset();
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
