#include <cstdint>

#include <gtest/gtest.h>

#include "bits_manipulation.hpp"

TEST(Bits_Manipulation, Mask_Bits)
{
    // whole word
    EXPECT_EQ((yarvs::mask_bits<15, 0>(uint16_t{0b1011011011101011})),
              uint16_t{0b1011011011101011});

    // middle of word
    EXPECT_EQ((yarvs::mask_bits<5, 2>(uint16_t{0b1011011011101011})),
              uint16_t{0b0000000000101000});

    // low part of word
    EXPECT_EQ((yarvs::mask_bits<4, 0>(uint16_t{0b1011011011101011})),
              uint16_t{0b01011});

    // high part of word
    EXPECT_EQ((yarvs::mask_bits<15, 5>(uint16_t{0b1011011011101011})),
              uint16_t{0b1011011011100000});
}

TEST(Bits_Manipulation, Get_Bits)
{
    // whole word
    EXPECT_EQ((yarvs::get_bits<15, 0>(uint16_t{0b1011011011101011})),
              uint16_t{0b1011011011101011});

    // middle of word
    EXPECT_EQ((yarvs::get_bits<5, 2>(uint16_t{0b1011011011101011})),
              uint16_t{0b1010});

    // low part of word
    EXPECT_EQ((yarvs::get_bits<4, 0>(uint16_t{0b1011011011101011})),
              uint16_t{0b01011});

    // high part of word
    EXPECT_EQ((yarvs::get_bits<15, 5>(uint16_t{0b1011011011101011})),
              uint16_t{0b10110110111});
}

TEST(Bits_Manipulation, Get_Bits_R)
{
    // whole word
    EXPECT_EQ((yarvs::get_bits_r<15, 0, uint16_t>(uint16_t{0b1011011011101011})),
              uint32_t{0b1011011011101011});

    // middle of word
    EXPECT_EQ((yarvs::get_bits_r<5, 2, uint16_t>(uint16_t{0b1011011011101011})),
              uint32_t{0b1010});

    // low part of word
    EXPECT_EQ((yarvs::get_bits_r<4, 0, uint16_t>(uint16_t{0b1011011011101011})),
              uint32_t{0b01011});

    // high part of word
    EXPECT_EQ((yarvs::get_bits_r<15, 5, uint16_t>(uint16_t{0b1011011011101011})),
              uint32_t{0b10110110111});
}

TEST(Bits_Manipulation, Mask_Bit)
{
    // first bit
    EXPECT_EQ((yarvs::mask_bit<0>(uint16_t{0b1011011011101010})), uint16_t{0});

    // last bit
    EXPECT_EQ((yarvs::mask_bit<15>(uint16_t{0b1011011011101010})), uint16_t{1 << 15});

    // middle bit
    EXPECT_EQ((yarvs::mask_bit<7>(uint16_t{0b1011011011101010})), uint16_t{1 << 7});
}

TEST(Bits_Manipulation, Sext)
{
    // 64 -> 64
    EXPECT_EQ((yarvs::sext<64, uint64_t>(uint64_t{0xFFFFFFFFFFFFFFFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<64, uint64_t>(uint64_t{0x7FFFFFFFFFFFFFFF})),
              0x7FFFFFFFFFFFFFFF);

    // 32 -> 64
    EXPECT_EQ((yarvs::sext<32, uint64_t>(uint32_t{0xFFFFFFFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<32, uint64_t>(uint32_t{0x7FFFFFFF})),
              0x000000007FFFFFFF);

    // 16 -> 64
    EXPECT_EQ((yarvs::sext<16, uint64_t>(uint16_t{0xFFFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<16, uint64_t>(uint16_t{0x7FFF})),
              0x0000000000007FFF);

    // 8 -> 64
    EXPECT_EQ((yarvs::sext<8, uint64_t>(uint8_t{0xFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<8, uint64_t>(uint8_t{0x7F})),
              0x000000000000007F);

    // 20 -> 64
    EXPECT_EQ((yarvs::sext<20, uint64_t>(uint32_t{0xFFFFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<20, uint64_t>(uint32_t{0x7FFFF})),
              0x000000000007FFFF);

    // 12 -> 64
    EXPECT_EQ((yarvs::sext<12, uint64_t>(uint16_t{0xFFF})),
              0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ((yarvs::sext<12, uint64_t>(uint16_t{0x7FF})),
              0x000000000007FF);
}
