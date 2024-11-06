#include <ranges>
#include <limits>

#include <gtest/gtest.h>

#include "common.hpp"
#include "hart.hpp"
#include "reg_file.hpp"

using namespace yarvs;

class ExecutorTest : public testing::Test
{
protected:

    static constexpr RawInstruction kEbreak = 0b00000000000100000000000001110011;
    static constexpr DoubleWord kEntry = 0x42;

    ExecutorTest() : hart{kEntry}
    {
        hart.memory().store(kEntry + sizeof(RawInstruction), kEbreak);
    }

    Hart hart;
};

TEST_F(ExecutorTest, Add_Two_Regs_Put_In_Third)
{
    // add x3, x1, x2
    hart.memory().store(kEntry, RawInstruction{0b0000000'00010'00001'000'00011'0110011});
    hart.gprs().set_reg(1, 15);
    hart.gprs().set_reg(2, 14);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 15);
    EXPECT_EQ(hart.gprs().get_reg(2), 14);
    EXPECT_EQ(hart.gprs().get_reg(3), 29);

    for (auto i : std::views::iota(4uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + sizeof(RawInstruction));
}

TEST_F(ExecutorTest, Add_Two_Regs_Put_In_First)
{
    // add x1, x1, x2
    hart.memory().store(kEntry, RawInstruction{0b0000000'00010'00001'000'00001'0110011});
    hart.gprs().set_reg(1, 15);
    hart.gprs().set_reg(2, 14);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 29);
    EXPECT_EQ(hart.gprs().get_reg(2), 14);

    for (auto i : std::views::iota(3uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + sizeof(RawInstruction));
}

TEST_F(ExecutorTest, Add_With_X0)
{
    // add x2, x0, x1
    hart.memory().store(kEntry, RawInstruction{0b0000000'00001'00000'000'00010'0110011});
    hart.gprs().set_reg(1, 15);
    hart.gprs().set_reg(2, 14);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 15);
    EXPECT_EQ(hart.gprs().get_reg(2), 15);

    for (auto i : std::views::iota(3uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + sizeof(RawInstruction));
}

TEST_F(ExecutorTest, Add_With_Overflow)
{
    // add x3, x1, x2
    hart.memory().store(kEntry, RawInstruction{0b0000000'00010'00001'000'00011'0110011});
    hart.gprs().set_reg(1, std::numeric_limits<DoubleWord>::max());
    hart.gprs().set_reg(2, 1);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), std::numeric_limits<DoubleWord>::max());
    EXPECT_EQ(hart.gprs().get_reg(2), 1);
    EXPECT_EQ(hart.gprs().get_reg(3), 0);

    for (auto i : std::views::iota(4uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + sizeof(RawInstruction));
}
