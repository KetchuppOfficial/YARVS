#ifndef INCLUDE_YARVS_ELF_LOADER_HPP
#define INCLUDE_YARVS_ELF_LOADER_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>

#include "yarvs/common.hpp"

namespace yarvs
{

class ELFLoader final
{
public:

    enum SegmentFlags : std::uint32_t
    {
        kExecute = 1,
        kWrite = 2,
        kRead = 4
    };

    ELFLoader(const std::filesystem::path &path);
    ~ELFLoader(); // defined elsewhere because of using pimpl

    DoubleWord get_entry() const;

    struct Segment final
    {
        const unsigned char *data;
        DoubleWord memory_size;
        DoubleWord file_size;
        DoubleWord virtual_address;
        bool loadable;
    };

    std::size_t segments_count() const;
    Segment segment(std::size_t i) const;

    std::map<DoubleWord, SegmentFlags> get_loadable_pages() const;

private:

    class ELFParser;

    std::unique_ptr<ELFParser> elf_;
};

constexpr ELFLoader::SegmentFlags operator|(ELFLoader::SegmentFlags lhs,
                                            ELFLoader::SegmentFlags rhs) noexcept
{
    using underlying_type = std::underlying_type_t<ELFLoader::SegmentFlags>;
    return ELFLoader::SegmentFlags{static_cast<underlying_type>(lhs) |
                                   static_cast<underlying_type>(rhs)};
}

} // namespace yarvs

#endif // INCLUDE_YARVS_ELF_LOADER_HPP
