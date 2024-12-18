#ifndef INCLUDE_CONFIG_HPP
#define INCLUDE_CONFIG_HPP

#include <algorithm>
#include <stdexcept>
#include <format>

#include "common.hpp"
#include "supervisor/satp.hpp"

namespace yarvs
{

struct Config final
{
public:

    static constexpr DoubleWord kMinStackPages = 4;

    Config(SATP::Mode translation_mode, DoubleWord n_stack_pages = 0)
        : translation_mode_{translation_mode},
          n_stack_pages_{std::max(n_stack_pages, kMinStackPages)}
    {
        switch (translation_mode_)
        {
            case SATP::Mode::kSv39:
                stack_top_ = 0x3ffff14000;
                break;
            case SATP::Mode::kSv48:
                stack_top_ = 0x7ffffff14000;
                break;
            case SATP::Mode::kSv57:
                stack_top_ = 0xfffffffff14000;
                break;
            default:
                throw std::invalid_argument{std::format("translation mode {} is not supported",
                                                        static_cast<DoubleWord>(translation_mode))};
        }
    }

    auto get_translation_mode() const noexcept { return translation_mode_; }
    auto get_n_stack_pages() const noexcept { return n_stack_pages_; }
    auto get_stack_top() const noexcept { return stack_top_; }

private:

    SATP::Mode translation_mode_;
    DoubleWord n_stack_pages_;
    DoubleWord stack_top_;
};

} // yarvs

#endif // INCLUDE_CONFIG_HPP
