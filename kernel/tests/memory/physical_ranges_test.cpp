#include "support/fixture.hpp"

namespace
{
void processorCpuid(uint32_t leaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    EXPECT_TRUE(leaf == 0x80000000 || leaf == 0x80000008);
    *a = leaf == 0x80000000 ? 0x80000008 : 48;
    *b = *c = *d = 0;
}

#include "production/physical_range.inc"

TEST(PhysicalRanges, RejectsEmptyOverflowingAndCpuUnaddressableSpans) {
    const uint64_t limit = 1ULL << 48;
    EXPECT_TRUE(memoryPhysicalIsRangeAddressable(0, 1));
    EXPECT_FALSE(memoryPhysicalIsRangeAddressable(0, 0));
    EXPECT_TRUE(memoryPhysicalIsRangeAddressable(0, limit));
    EXPECT_TRUE(memoryPhysicalIsRangeAddressable(1, limit - 1));
    EXPECT_TRUE(memoryPhysicalIsRangeAddressable(limit - 1, 1));
    EXPECT_FALSE(memoryPhysicalIsRangeAddressable(limit - 1, 2));
    EXPECT_FALSE(memoryPhysicalIsRangeAddressable(limit, 1));
    EXPECT_FALSE(memoryPhysicalIsRangeAddressable(1, UINT64_MAX));
    EXPECT_FALSE(memoryPhysicalIsRangeAddressable(UINT64_MAX, 2));
}
}
