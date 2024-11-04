#ifndef INCLUDE_MEMORY_MMAP_WRAPPER_HPP
#define INCLUDE_MEMORY_MMAP_WRAPPER_HPP

#include <cstdint>
#include <cstddef>
#include <memory>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <format>
#include <type_traits>

#include <sys/mman.h>

namespace yarvs
{

class MMapWrapper final
{
    class Unmapper final // no EBO can be done, cause we store a member
    {
    public:
        explicit Unmapper(std::size_t size) noexcept : size_{size} {}

        void operator()(std::uint8_t *ptr) const noexcept
        {
            [[maybe_unused]] int res = munmap(ptr, size_);
            assert(res == 0);
        }

    private:
        std::size_t size_;
    };

public:

    enum ProtMode : std::uint8_t
    {
        kNone = PROT_NONE,
        kRead = PROT_READ,
        kWrite = PROT_WRITE,
        kExec = PROT_EXEC
    };

    MMapWrapper(std::size_t len, ProtMode prot) : mem_{perform_map(len, prot), Unmapper{len}} {}

    const std::uint8_t &operator[](std::size_t i) const noexcept { return mem_[i]; }
    std::uint8_t &operator[](std::size_t i) noexcept { return mem_[i]; }

private:

    /*
     * From man about MAP_ANONYMOUS flag:
     *
     * The mapping is not backed by any file; its contents are initialized to zero. The fd argument
     * is ignored; however, some implementations require fd to be -1 if MAP_ANONYMOUS (or MAP_ANON)
     * is specified, and portable applications should ensure this. The offset argument should be
     * zero.
     */
    std::uint8_t *perform_map(std::size_t len, ProtMode prot)
    {
        auto ptr = static_cast<std::uint8_t *>(mmap(NULL, len, prot, MAP_PRIVATE | MAP_ANONYMOUS,
                                                    -1 /* fd */, 0 /* offset */));
        if (ptr == MAP_FAILED) [[unlikely]]
            throw std::runtime_error{std::format("mmap failed: {}", std::strerror(errno))};
        return ptr;
    }

    std::unique_ptr<std::uint8_t[], Unmapper> mem_;
};

constexpr MMapWrapper::ProtMode operator|(MMapWrapper::ProtMode lhs, MMapWrapper::ProtMode rhs)
{
    using underlying_type = std::underlying_type_t<MMapWrapper::ProtMode>;
    return static_cast<MMapWrapper::ProtMode>(static_cast<underlying_type>(lhs) |
                                              static_cast<underlying_type>(rhs));
}

} // namespace yarvs

#endif // INCLUDE_MEMORY_MMAP_WRAPPER_HPP
