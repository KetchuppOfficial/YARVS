#ifndef INCLUDE_MEMORY_MEMORY_HPP
#define INCLUDE_MEMORY_MEMORY_HPP

#include <cstddef>
#include <expected>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

#include "yarvs/bits_manipulation.hpp"
#include "yarvs/common.hpp"
#include "yarvs/memory/mmap_wrapper.hpp"
#include "yarvs/memory/virtual_address.hpp"
#include "yarvs/memory/pte.hpp"
#include "yarvs/privileged/cs_regfile.hpp"

namespace yarvs
{

class Memory final
{
public:

    static constexpr DoubleWord kPageBits = 12;
    static constexpr DoubleWord kPageSize = 1 << kPageBits;
    static constexpr std::size_t kPhysMemAmount = 4 * (std::size_t{1} << 30); // 4GB

    explicit Memory(CSRegFile &csrs, const PrivilegeLevel &priv_mode)
        : physical_mem_{kPhysMemAmount, MMapWrapper::kRead | MMapWrapper::kWrite},
          csrs_{csrs}, priv_level_{priv_mode} {}

    template<riscv_type T>
    std::expected<T, MCause::Exception> load(DoubleWord va)
    {
        if (!csrs_.is_satp_active(priv_level_))
            return pm_load<T>(va);
        const auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
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
            const auto maybe_pa = translate_address<MemoryAccessType::kWrite>(va);
            if (!maybe_pa.has_value()) [[unlikely]]
                return std::unexpected{MCause::Exception::kStoreAMOPageFault};
            pm_store(*maybe_pa, value);
        }
        return {};
    }

    template<std::input_iterator It>
    requires riscv_type<std::remove_const_t<typename std::iterator_traits<It>::value_type>>
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
        const auto maybe_pa = translate_address<MemoryAccessType::kExecute>(va);
        if (!maybe_pa.has_value()) [[unlikely]]
            return std::unexpected{MCause::Exception::kInstrPageFault};
        return pm_load<RawInstruction>(*maybe_pa);
    }

    std::expected<const Byte *, MCause::Exception> host_ptr(DoubleWord va)
    {
        if (!csrs_.is_satp_active(priv_level_))
            return &physical_mem_[va];
        const auto maybe_pa = translate_address<MemoryAccessType::kRead>(va);
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
    std::optional<DoubleWord> translate_address(DoubleWord va)
    {
        switch (csrs_.get_satp().get_mode())
        {
            case SATP::Mode::kBare:
                return va;
            case SATP::Mode::kSv39:
                if (va != sext<39, DoubleWord>(va))
                    return std::nullopt;
                return translate_address<kAccessKind, /*kLevels=*/3>(va);
            case SATP::Mode::kSv48:
                if (va != sext<48, DoubleWord>(va))
                    return std::nullopt;
                return translate_address<kAccessKind, /*kLevels=*/4>(va);
            case SATP::Mode::kSv57:
                if (va != sext<57, DoubleWord>(va))
                    return std::nullopt;
                return translate_address<kAccessKind, /*kLevels=*/5>(va);
            default: [[unlikely]]
                std::unreachable();
        }
    }

    template<MemoryAccessType kAccessKind, Byte kLevels>
    std::optional<DoubleWord> translate_address(VirtualAddress va)
    {
        static_assert(3 <= kLevels && kLevels <= 5);

        const MStatus mstatus = csrs_.get_mstatus();
        DoubleWord a = csrs_.get_satp().get_ppn() * kPageSize;

        PTE pte;
        auto i = kLevels - 1;
        for (;;)
        {
            const DoubleWord pa = a + va.get_vpn(i) * sizeof(PTE);
            pte = pm_load<DoubleWord>(pa);

            if (!pte.get_V() || pte.is_rwx_reserved() || pte.uses_reserved())
                return std::nullopt;

            // PTE is valid

            if (pte.is_pointer_to_next_level_pte())
            {
                if (i == 0)
                    return std::nullopt;
                --i;
                a = pte.get_whole_ppn();
                continue;
            }

            // A leaf PTE has been found

            if constexpr (kAccessKind == MemoryAccessType::kRead)
            {
                if (mstatus.get_mxr())
                {
                    if (!pte.get_R() && !pte.get_E())
                        return std::nullopt;
                }
                else if (!pte.get_R())
                    return std::nullopt;
            }
            else if constexpr (kAccessKind == MemoryAccessType::kWrite)
            {
                if (!pte.get_W())
                    return std::nullopt;
            }
            else
            {
                if (!pte.get_E())
                    return std::nullopt;
            }

            if (priv_level_ == PrivilegeLevel::kSupervisor && pte.get_U() && !mstatus.get_sum())
                return std::nullopt;

            if (i > 0 && pte.get_lower_ppn<kLevels>(i - 1)) // misaligned superpage
                return std::nullopt;

            if constexpr (kAccessKind == MemoryAccessType::kWrite) {
                if (!pte.get_A() || !pte.get_D())
                {
                    if (pte != pm_load<DoubleWord>(pa))
                        continue;

                    pte.set_A(true);
                    pte.set_D(true);
                }
            } else {
                if (!pte.get_A())
                {
                    if (pte != pm_load<DoubleWord>(pa))
                        continue;

                    pte.set_A(true);
                }
            }

            pm_store(pa, +pte);

            DoubleWord result = pte.get_upper_ppn<kLevels>(i) | va.get_page_offset();
            if (i > 0) // superpage translation
                result |= pte.get_lower_ppn<kLevels>(i - 1);
            return result;
        }
    }

    template<riscv_type T>
    T pm_load(DoubleWord pa) const { return *reinterpret_cast<const T *>(&physical_mem_[pa]); }

    template<riscv_type T>
    void pm_store(DoubleWord pa, T value) { *reinterpret_cast<T*>(&physical_mem_[pa]) = value; }

    MMapWrapper physical_mem_;
    CSRegFile &csrs_;
    const PrivilegeLevel &priv_level_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_MEMORY_HPP
