#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL
#include "production/memorySizeToPageCount.inc"

TEST(PageCounts, RoundsUpWithin32BitPageCountAndRejectsOverflow)
{
    uint32_t pages = 99;
    EXPECT_TRUE(!memorySizeToPageCount(0, &pages) && !pages);
    EXPECT_TRUE(memorySizeToPageCount(4097, &pages) && pages == 2);
    EXPECT_TRUE(memorySizeToPageCount(UINT64_C(0xffffffff) * 4096, &pages) && pages == UINT32_MAX);
    EXPECT_TRUE(!memorySizeToPageCount(UINT64_C(1) << 44, &pages) && !pages);
    EXPECT_TRUE(!memorySizeToPageCount(UINT64_MAX, &pages) && !pages);
}
}
