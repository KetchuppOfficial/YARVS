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
