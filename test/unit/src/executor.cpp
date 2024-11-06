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

TEST_F(ExecutorTest, Add)
{
    constexpr DoubleWord kMaxDoubleWord = 0xffffffffffffffff;
    constexpr std::array<RawInstruction, 4> kInstructions = {
        0b0000000'00010'00001'000'00011'0110011, // add x3, x1, x2 [rd != rs1 && rd != rs2]
        0b0000000'00101'00100'000'00100'0110011, // add x4, x4, x5 [rd == rs1]
        0b0000000'00101'00000'000'00110'0110011, // add x6, x0, x5 [rs1 == x0]
        0b0000000'01000'00111'000'01001'0110011  // add x9, x7, x8 [overflow]
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, 1);
    hart.gprs().set_reg(2, 3);
    hart.gprs().set_reg(4, 5);
    hart.gprs().set_reg(5, 7);
    hart.gprs().set_reg(7, kMaxDoubleWord);
    hart.gprs().set_reg(8, 1);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 1);
    EXPECT_EQ(hart.gprs().get_reg(2), 3);
    EXPECT_EQ(hart.gprs().get_reg(3), 4);
    EXPECT_EQ(hart.gprs().get_reg(4), 12);
    EXPECT_EQ(hart.gprs().get_reg(5), 7);
    EXPECT_EQ(hart.gprs().get_reg(6), 7);
    EXPECT_EQ(hart.gprs().get_reg(7), kMaxDoubleWord);
    EXPECT_EQ(hart.gprs().get_reg(8), 1);
    EXPECT_EQ(hart.gprs().get_reg(9), 0);

    for (auto i : std::views::iota(10uz, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}

TEST_F(ExecutorTest, Sub)
{
    constexpr std::array<RawInstruction, 5> kInstructions = {
        0b0100000'00010'00001'000'00011'0110011, // sub x3, x1, x2 [rd != rs1 && rd != rs2]
        0b0100000'00101'00100'000'00100'0110011, // sub x4, x4, x5 [rd == rs1]
        0b0100000'00101'00000'000'00110'0110011, // sub x6, x0, x5 [rs1 == x0]
        0b0100000'00000'00101'000'00111'0110011, // sub x7, x5, x0 [rs1 == x0]
        0b0100000'01001'01000'000'01010'0110011  // sub x10, x8, x9 [overflow]
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, 15);
    hart.gprs().set_reg(2, 13);
    hart.gprs().set_reg(4, 42);
    hart.gprs().set_reg(5, 7);
    hart.gprs().set_reg(8, 1);
    hart.gprs().set_reg(9, 2);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 15);
    EXPECT_EQ(hart.gprs().get_reg(2), 13);
    EXPECT_EQ(hart.gprs().get_reg(3), 2);
    EXPECT_EQ(hart.gprs().get_reg(4), 35);
    EXPECT_EQ(hart.gprs().get_reg(5), 7);
    EXPECT_EQ(hart.gprs().get_reg(6), -7);
    EXPECT_EQ(hart.gprs().get_reg(7), 7);
    EXPECT_EQ(hart.gprs().get_reg(8), 1);
    EXPECT_EQ(hart.gprs().get_reg(9), 2);
    EXPECT_EQ(hart.gprs().get_reg(10), -1);

    for (auto i : std::views::iota(11uz, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}

TEST_F(ExecutorTest, Addi)
{
    constexpr std::array<RawInstruction, 7> kInstructions = {
        0b000000000000'00001'000'00010'0010011, // addi x2, x1, 0  [rd != rs1 && imm == 0] [mv x2, x1]
        0b000000000001'00001'000'00011'0010011, // addi x3, x1, 1  [rd != rs1 && imm > 0]
        0b111111111111'00001'000'00100'0010011, // addi x4, x1, -1 [rd != rs1 && imm < 0]
        0b000000000000'00101'000'00101'0010011, // addi x5, x5, 0  [rd == rs1 && imm == 0]
        0b000000000001'00110'000'00110'0010011, // addi x6, x6, 1  [rd == rs1 && imm > 0]
        0b111111111111'00111'000'00111'0010011, // addi x7, x7, -1 [rd == rs1 && imm < 0]
        0b000000000010'01000'000'01000'0010011  // addi x8, x8, 2  [overflow]
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, 42);
    hart.gprs().set_reg(5, 17);
    hart.gprs().set_reg(6, 10);
    hart.gprs().set_reg(7, 14);
    hart.gprs().set_reg(8, 0xfffffffffffffffe);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), 42);
    EXPECT_EQ(hart.gprs().get_reg(2), 42);
    EXPECT_EQ(hart.gprs().get_reg(3), 43);
    EXPECT_EQ(hart.gprs().get_reg(4), 41);
    EXPECT_EQ(hart.gprs().get_reg(5), 17);
    EXPECT_EQ(hart.gprs().get_reg(6), 11);
    EXPECT_EQ(hart.gprs().get_reg(7), 13);
    EXPECT_EQ(hart.gprs().get_reg(8), 0);

    for (auto i : std::views::iota(9uz, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}

TEST_F(ExecutorTest, Load_Byte)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Byte, 2> kValues = {0x7f, 0xff};
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b000000000000'00001'000'00010'0000011, // lb x2, 0(x1)
        0b000000000001'00001'000'00011'0000011  // lb x3, 1(x1)
    };

    add_instructions(kInstructions);

    hart.memory().store(kAddr, kValues.begin(), kValues.end());
    hart.gprs().set_reg(1, kAddr);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);
    EXPECT_EQ(hart.gprs().get_reg(2), DoubleWord{0x00'00'00'00'00'00'00'7f});
    EXPECT_EQ(hart.gprs().get_reg(3), DoubleWord{0xff'ff'ff'ff'ff'ff'ff'ff});

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<Byte>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Byte>(kAddr + sizeof(Byte)), kValues[1]);
}

TEST_F(ExecutorTest, Load_HalfWord)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<HalfWord, 2> kValues = {0x7fff, 0xffff};
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b000000000000'00001'001'00010'0000011, // lh x2, 0(x1)
        0b000000000010'00001'001'00011'0000011  // lh x3, 2(x1)
    };

    add_instructions(kInstructions);

    hart.memory().store(kAddr, kValues.begin(), kValues.end());
    hart.gprs().set_reg(1, kAddr);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);
    EXPECT_EQ(hart.gprs().get_reg(2), DoubleWord{0x0000'0000'0000'7fff});
    EXPECT_EQ(hart.gprs().get_reg(3), DoubleWord{0xffff'ffff'ffff'ffff});

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr + sizeof(HalfWord)), kValues[1]);
}

TEST_F(ExecutorTest, Load_Word)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Word, 2> kValues = {0x7fffffff, 0xffffffff};
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b000000000000'00001'010'00010'0000011, // lw x2, 0(x1)
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

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<Word>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + sizeof(Word)), kValues[1]);
}

TEST_F(ExecutorTest, Load_DoubleWord)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<DoubleWord, 2> kValues = {
        0x7fffffffffffffff, 0xffffffffffffffff
    };
    constexpr std::array<RawInstruction, 2> kInstructions = {
        0b000000000000'00001'011'00010'0000011, // ld x2, 0(x1)
        0b000000001000'00001'011'00011'0000011  // ld x3, 8(x1)
    };

    add_instructions(kInstructions);

    hart.memory().store(kAddr, kValues.begin(), kValues.end());
    hart.gprs().set_reg(1, kAddr);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);
    EXPECT_EQ(hart.gprs().get_reg(2), kValues[0]);
    EXPECT_EQ(hart.gprs().get_reg(3), kValues[1]);

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr + sizeof(DoubleWord)), kValues[1]);
}

TEST_F(ExecutorTest, Store_Byte)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Byte, 4> kValues = {
        0x7f, 0xff, 0x70, 0xf0
    };
    constexpr std::array<RawInstruction, 4> kInstructions = {
        0b0000000'00010'00001'000'00000'0100011, // sb x2, 0(x1)
        0b0000000'00011'00001'000'00001'0100011, // sb x3, 1(x1)
        0b0000001'00100'00001'000'00000'0100011, // sb x4, 32(x1)
        0b0000001'00101'00001'000'00001'0100011  // sb x5, 33(x1)
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, kAddr);
    for (auto [i, v] : std::views::enumerate(kValues))
        hart.gprs().set_reg(i + 2, v);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);

    for (auto [i, v] : std::views::enumerate(kValues))
        EXPECT_EQ(hart.gprs().get_reg(i + 2), v) << std::format("x{}", i);

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<Byte>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Byte>(kAddr + sizeof(Byte)), kValues[1]);
    EXPECT_EQ(hart.memory().load<Byte>(kAddr + 32), kValues[2]);
    EXPECT_EQ(hart.memory().load<Byte>(kAddr + 32 + sizeof(Byte)), kValues[3]);
}

TEST_F(ExecutorTest, Store_HalfWord)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<HalfWord, 4> kValues = {
        0x7fff, 0xffff, 0x7ff0, 0xfff0
    };
    constexpr std::array<RawInstruction, 4> kInstructions = {
        0b0000000'00010'00001'001'00000'0100011, // sh x2, 0(x1)
        0b0000000'00011'00001'001'00010'0100011, // sh x3, 2(x1)
        0b0000001'00100'00001'001'00000'0100011, // sh x4, 32(x1)
        0b0000001'00101'00001'001'00010'0100011  // sh x5, 34(x1)
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, kAddr);
    for (auto [i, v] : std::views::enumerate(kValues))
        hart.gprs().set_reg(i + 2, v);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);

    for (auto [i, v] : std::views::enumerate(kValues))
        EXPECT_EQ(hart.gprs().get_reg(i + 2), v) << std::format("x{}", i);

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr + sizeof(HalfWord)), kValues[1]);
    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr + 32), kValues[2]);
    EXPECT_EQ(hart.memory().load<HalfWord>(kAddr + 32 + sizeof(HalfWord)), kValues[3]);
}

TEST_F(ExecutorTest, Store_Word)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<Word, 4> kValues = {
        0x7fffffff, 0xffffffff, 0x7ffffff0, 0xfffffff0
    };
    constexpr std::array<RawInstruction, 4> kInstructions = {
        0b0000000'00010'00001'010'00000'0100011, // sw x2, 0(x1)
        0b0000000'00011'00001'010'00100'0100011, // sw x3, 4(x1)
        0b0000001'00100'00001'010'00000'0100011, // sw x4, 32(x1)
        0b0000001'00101'00001'010'00100'0100011  // sw x5, 36(x1)
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, kAddr);
    for (auto [i, v] : std::views::enumerate(kValues))
        hart.gprs().set_reg(i + 2, v);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);

    for (auto [i, v] : std::views::enumerate(kValues))
        EXPECT_EQ(hart.gprs().get_reg(i + 2), v) << std::format("x{}", i);

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<Word>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + sizeof(Word)), kValues[1]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + 32), kValues[2]);
    EXPECT_EQ(hart.memory().load<Word>(kAddr + 32 + sizeof(Word)), kValues[3]);
}

TEST_F(ExecutorTest, Store_DoubleWord)
{
    constexpr auto kAddr = kEntry + 0x1000;
    constexpr std::array<DoubleWord, 4> kValues = {
        0x7fffffffffffffff, 0xffffffffffffffff,
        0x7ffffffffffffff0, 0xfffffffffffffff0
    };
    constexpr std::array<RawInstruction, 4> kInstructions = {
        0b0000000'00010'00001'011'00000'0100011, // sd x2, 0(x1)
        0b0000000'00011'00001'011'01000'0100011, // sd x3, 8(x1)
        0b0000001'00100'00001'011'00000'0100011, // sd x4, 32(x1)
        0b0000001'00101'00001'011'01000'0100011  // sd x5, 40(x1)
    };

    add_instructions(kInstructions);

    hart.gprs().set_reg(1, kAddr);
    for (auto [i, v] : std::views::enumerate(kValues))
        hart.gprs().set_reg(i + 2, v);

    hart.run();

    EXPECT_EQ(hart.gprs().get_reg(0), 0);
    EXPECT_EQ(hart.gprs().get_reg(1), kAddr);

    for (auto [i, v] : std::views::enumerate(kValues))
        EXPECT_EQ(hart.gprs().get_reg(i + 2), v) << std::format("x{}", i);

    for (auto i : std::views::iota(kValues.size() + 2, RegFile::kNRegs))
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);

    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr), kValues[0]);
    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr + sizeof(DoubleWord)), kValues[1]);
    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr + 32), kValues[2]);
    EXPECT_EQ(hart.memory().load<DoubleWord>(kAddr + 32 + sizeof(DoubleWord)), kValues[3]);
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
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

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
        EXPECT_EQ(hart.gprs().get_reg(i), 0) << std::format("x{}", i);

    EXPECT_EQ(hart.get_pc(), kEntry + kInstructions.size() * kInstrSize);
}
