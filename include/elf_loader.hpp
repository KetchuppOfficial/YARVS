#ifndef INCLUDE_ELF_LOADER_HPP
#define INCLUDE_ELF_LOADER_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>
#include <filesystem>

#include <elfio/elfio.hpp>

namespace yarvs
{

class LoadableSegment final : private std::vector<uint8_t>
{
    using Base = std::vector<uint8_t>;

public:

    static constexpr auto kExecutable = ELFIO::PF_X;
    static constexpr auto kWritable = ELFIO::PF_W;
    static constexpr auto kReadable = ELFIO::PF_R;

    LoadableSegment(const uint8_t *data, std::size_t file_size, std::size_t memory_size,
                     std::size_t v_addr, ELFIO::Elf_Word flags)
        : Base(data, data + file_size),
          memory_size_{memory_size}, v_addr_{v_addr}, flags_{flags} {}

    using Base::iterator;
    using Base::const_iterator;
    using Base::begin;
    using Base::end;
    using Base::cbegin;
    using Base::cend;

    std::size_t get_file_size() const noexcept { return size(); }
    std::size_t get_memory_size() const noexcept { return memory_size_; }

    std::size_t get_virtual_addr() const noexcept { return v_addr_; }

    bool is_executable() const noexcept { return flags_ & kExecutable; }
    bool is_writable() const noexcept { return flags_ & kWritable; }
    bool is_readable() const noexcept { return flags_ & kReadable; }

private:

    std::size_t memory_size_;
    std::size_t v_addr_;
    ELFIO::Elf_Word flags_;
};

class LoadableImage final : private std::vector<LoadableSegment>
{
    using Base = std::vector<LoadableSegment>;

public:

    explicit LoadableImage(const std::string &path_str);
    explicit LoadableImage(const std::filesystem::path &path) : LoadableImage{path.native()} {}

    using Base::size;
    using Base::operator[];
    using Base::at;
    using Base::begin;
    using Base::end;
    using Base::cbegin;
    using Base::cend;

    std::size_t get_entry_point() const { return entry_; }

private:

    std::size_t entry_;
};

} // namespace yarvs

#endif // INCLUDE_ELF_LOADER_HPP
