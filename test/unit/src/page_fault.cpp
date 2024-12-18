#include <gtest/gtest.h>
#include <stdexcept>

#include "common.hpp"
#include "config.hpp"
#include "hart.hpp"

using namespace yarvs;

class PageFaultTest : public testing::Test
{
protected:

    static constexpr DoubleWord kMagicVA = 0x42;

    PageFaultTest() : hart{Config{SATP::Mode::kSv39}} {}

    Hart hart;
};

TEST_F(PageFaultTest, Load)
{
    EXPECT_THROW(hart.memory().load<Byte>(kMagicVA), std::runtime_error);
    EXPECT_THROW(hart.memory().load<HalfWord>(kMagicVA), std::runtime_error);
    EXPECT_THROW(hart.memory().load<Word>(kMagicVA), std::runtime_error);
    EXPECT_THROW(hart.memory().load<DoubleWord>(kMagicVA), std::runtime_error);
}

TEST_F(PageFaultTest, Store)
{
    EXPECT_THROW(hart.memory().store(kMagicVA, Byte{42}), std::runtime_error);
    EXPECT_THROW(hart.memory().store(kMagicVA, HalfWord{42}), std::runtime_error);
    EXPECT_THROW(hart.memory().store(kMagicVA, Word{42}), std::runtime_error);
    EXPECT_THROW(hart.memory().store(kMagicVA, DoubleWord{42}), std::runtime_error);
}
