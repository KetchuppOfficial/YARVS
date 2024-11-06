#include <array>
#include <ranges>

#include <gtest/gtest.h>

#include "common.hpp"
#include "hart.hpp"
#include "reg_file.hpp"

using namespace yarvs;

class ExecutorTest : public testing::Test
{
protected:

    static constexpr RawInstruction kEbreak = 0b00000000000100000000000001110011;
    static constexpr DoubleWord kEntry = 0x42000;
    static constexpr DoubleWord kPageSize = 0x1000;
    static constexpr auto kInstrSize = sizeof(RawInstruction);

    ExecutorTest() : hart{kEntry} {}

    void add_instruction(RawInstruction instr)
    {
        hart.memory().store(kEntry, instr);
        hart.memory().store(kEntry + kInstrSize, kEbreak);
    }

    template<std::ranges::forward_range R>
    requires std::is_same_v<std::ranges::range_value_t<R>, RawInstruction>
    void add_instructions(R &&instructions)
    {
        namespace ranges = std::ranges;

        hart.memory().store(kEntry, ranges::begin(instructions), ranges::end(instructions));
        hart.memory().store(kEntry + ranges::distance(instructions) * kInstrSize, kEbreak);
    }

    Hart hart;
};

TEST_F(ExecutorTest, Add_Two_Regs_Put_In_Third)
{
    // add x3, x1, x2
    add_instruction(0b0000000'00010'00001'000'00011'0110011);

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
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstrSize);
}

TEST_F(ExecutorTest, Add_Two_Regs_Put_In_First)
{
    // add x1, x1, x2
    add_instruction(0b0000000'00010'00001'000'00001'0110011);

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

    EXPECT_EQ(hart.get_pc(), kEntry + kInstrSize);
}

TEST_F(ExecutorTest, Add_With_X0)
{
    // add x2, x0, x1
    add_instruction(0b0000000'00001'00000'000'00010'0110011);

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

    EXPECT_EQ(hart.get_pc(), kEntry + kInstrSize);
}

TEST_F(ExecutorTest, Add_With_Overflow)
{
    constexpr DoubleWord kMaxDoubleWord = 0xffffffffffffffff;

    // add x3, x1, x2
    add_instruction(0b0000000'00010'00001'000'00011'0110011);

    hart.gprs().set_reg(1, kMaxDoubleWord);
    hart.gprs().set_reg(2, 1);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kMaxDoubleWord);
    EXPECT_EQ(hart.gprs().get_reg(2), 1);
    EXPECT_EQ(hart.gprs().get_reg(3), 0);

    for (auto i : std::views::iota(4uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstrSize);
}

TEST_F(ExecutorTest, Load_Word)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Word, 2> kValues = {0x7fffffff, 0xffffffff};
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b000000000000'00001'010'00010'0000011, // lw x2, x1
        0b000000000100'00001'010'00011'0000011  // lw x3, 4(x1)
    };

    add_instructions(kInstructions);

    hart.memory().store(kAddr, kValues.begin(), kValues.end());
    hart.gprs().set_reg(1, kAddr);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);
    EXPECT_EQ(hart.gprs().get_reg(2), DoubleWord{0x00000000'7fffffff});
    EXPECT_EQ(hart.gprs().get_reg(3), DoubleWord{0xffffffff'ffffffff});

    for (auto i : std::views::iota(4uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{}", i);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<Word>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + 4), kValues[1]);
}

TEST_F(ExecutorTest, Store_Word)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Word, 2> kValues = {0x7fffffff, 0xffffffff};
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b0000000'00010'00001'010'00000'0100011, // sw x2, x1
        0b0000000'00011'00001'010'00100'0100011  // sw x3, 4(x1)
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, kAddr);
    hart.gprs().set_reg(2, kValues[0]);
    hart.gprs().set_reg(3, kValues[1]);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);
    EXPECT_EQ(hart.gprs().get_reg(2), kValues[0]);
    EXPECT_EQ(hart.gprs().get_reg(3), kValues[1]);

    for (auto i : std::views::iota(4uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() *  kInstrSize);

    EXPECT_EQ(hart.memory().load<Word>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + 4), kValues[1]);
}

TEST_F(ExecutorTest, Auipc)
{
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b00000000000000000100'00001'0010111, // auipc x1, 4
        0b11111111111111111100'00010'0010111  // auipc x2, -4
    };

    add_instructions(kInstructions);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kEntry + 4 * kPageSize);
    EXPECT_EQ(hart.gprs().get_reg(2), kEntry + kInstrSize - 4 * kPageSize);

    for (auto i : std::views::iota(3uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}

TEST_F(ExecutorTest, Lui)
{
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b00000000000000000001'00001'0110111, // lui x1, 1
        0b11111111111111111111'00010'0110111, // lui x2, -1
    };

    add_instructions(kInstructions);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kPageSize);
    EXPECT_EQ(hart.gprs().get_reg(2), -kPageSize);

    for (auto i : std::views::iota(3uz, RegFile::kNRegs))
    {
        auto reg = hart.gprs().get_reg(i);
        EXPECT_EQ(reg, 0) << std::format("x{} == {:#x}", i, reg);
    }

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}
