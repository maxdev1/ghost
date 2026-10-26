#include "support/fixture.hpp"

namespace
{
#include "production/math_cpp.inc"

TEST(PowerOfTwo, Rounds64BitValuesWithoutTruncatingHighBits)
{
    EXPECT_EQ(0x100000000ULL, mathCeilToPowerOf2(0x80000001ULL));
    EXPECT_EQ(1ULL << 41, mathCeilToPowerOf2((1ULL << 40) + 1));
    EXPECT_EQ(1U, mathCeilToPowerOf2(0));
    EXPECT_EQ(1U, mathCeilToPowerOf2(1));
    EXPECT_EQ(16U, mathCeilToPowerOf2(16));
    EXPECT_EQ(32U, mathCeilToPowerOf2(17));
}
}
