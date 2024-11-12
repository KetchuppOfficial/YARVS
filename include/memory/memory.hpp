#ifndef INCLUDE_MEMORY_MEMORY_HPP
#define INCLUDE_MEMORY_MEMORY_HPP

#include <cstddef>
#include <iterator>
#include <algorithm>

#include "common.hpp"
#include "memory/mmap_wrapper.hpp"

namespace yarvs
{

class Memory final
{
public:

    static constexpr std::size_t kPhysMemAmount = 4 * (std::size_t{1} << 30); // 4GB

    Memory() : physical_mem_{kPhysMemAmount, MMapWrapper::kRead | MMapWrapper::kWrite} {}

    template<riscv_type T>
    T load(DoubleWord addr) const
    {
        // temporary implementation without virtual memory
        return *reinterpret_cast<const T *>(ptr(addr));
    }

    template<riscv_type T>
    void store(DoubleWord addr, T value)
    {
        // temporary implementation without virtual memory
        *reinterpret_cast<T*>(ptr(addr)) = value;
    }

    template<std::forward_iterator It>
    void store(DoubleWord addr, It first, It last)
    {
        using value_type = typename std::iterator_traits<It>::value_type;
        std::copy(first, last, reinterpret_cast<value_type *>(ptr(addr)));
    }

    const uint8_t *ptr(DoubleWord addr) const { return &physical_mem_[addr]; }
    uint8_t *ptr(DoubleWord addr) { return &physical_mem_[addr]; }

private:

    MMapWrapper physical_mem_;
};

} // namespace yarvs

#endif // INCLUDE_MEMORY_MEMORY_HPP
