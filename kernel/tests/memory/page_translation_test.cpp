#include "support/fixture.hpp"

namespace
{
#include "support/memory/page_tables.hpp"

TEST(PageTables, MasksParentFlagsAndResolvesHugeAndOrdinaryLeaves)
{
    alignas(4096) uint64_t root[512] = {};
    currentSpace = reinterpret_cast<uintptr_t>(root);
    // Preserve leaf flags, mask NX in parent pointers, and stop at huge leaves.
    alignas(4096) uint64_t pdpt[512] = {}, pd[512] = {}, pt[512] = {};
    std::memset(root, 0, sizeof(root));
    root[0] = (uintptr_t)pdpt | G_PAGE_NX_FLAG | 7;
    pdpt[0] = 0x80000000ULL | G_PAGE_NX_FLAG | G_PAGE_LARGE_PAGE_FLAG | 7;
    EXPECT_EQ(pdpt[0], pagingVirtualToPageEntry(0x1234));
    EXPECT_EQ(0x80001234ULL, pagingVirtualToPhysical(0x1234));
    EXPECT_FALSE(pagingMapPage(0x1000, 0x9000, 7, 7));
    pdpt[0] = (uintptr_t)pd | G_PAGE_NX_FLAG | 7;
    pd[0] = 0xa0000000ULL | G_PAGE_NX_FLAG | G_PAGE_LARGE_PAGE_FLAG | 7;
    EXPECT_EQ(pd[0], pagingVirtualToPageEntry(0x1234));
    EXPECT_EQ(0xa0001234ULL, pagingVirtualToPhysical(0x1234));
    EXPECT_FALSE(pagingMapPage(0x1000, 0x9000, 7, 7));
    pd[0] = (uintptr_t)pt | G_PAGE_NX_FLAG | 7;
    pt[1] = 0x100000000ULL | G_PAGE_NX_FLAG | 7;
    EXPECT_EQ(pt[1], pagingVirtualToPageEntry(0x1234));
    EXPECT_EQ(0x100000234ULL, pagingVirtualToPhysical(0x1234));
    pagingUnmapPage(0x1000); EXPECT_FALSE(pagingVirtualToPageEntry(0x1234));
    pt[1] = 0x100000000ULL; // non-present entry carrying a physical address
    EXPECT_TRUE(!pagingVirtualToPageEntry(0x1234) && !pagingVirtualToPhysical(0x1234));
}

TEST(PageTables, BatchUnmapReturnsFramesOnlyAfterOneRangeSynchronization)
{
    alignas(4096) uint64_t root[512] = {}, pdpt[512] = {}, pd[512] = {}, pt[512] = {};
    currentSpace = (uintptr_t)root;
    root[0] = (uintptr_t)pdpt | 7; pdpt[0] = (uintptr_t)pd | 7; pd[0] = (uintptr_t)pt | 7;
    pt[1] = 0x100000000ULL | G_PAGE_NX_FLAG | 7;
    pt[3] = 0x100002000ULL | 7;
    uintptr_t retired[4];
    synchronizedRanges = 0;
    pagingUnmapPages(0x1000, 4, retired);
    EXPECT_EQ(1u, synchronizedRanges); EXPECT_EQ(0x1000u, synchronizedAddress);
    EXPECT_EQ(4u, synchronizedPages);
    EXPECT_FALSE(synchronizedGlobal);
    EXPECT_EQ(0x100000000ULL | G_PAGE_NX_FLAG | 7, retired[0]); EXPECT_EQ(0u, retired[1]);
    EXPECT_EQ(0x100002000ULL | 7, retired[2]); EXPECT_EQ(0u, retired[3]);
    for(unsigned i = 1; i <= 4; ++i) EXPECT_EQ(0u, pt[i]);
    pagingUnmapPages(0x1000, 4, retired);
    EXPECT_EQ(1u, synchronizedRanges);
    pt[1] = 7; // Physical frame zero still requires invalidation.
    pagingUnmapPages(0x1000, 1, retired);
    EXPECT_EQ(2u, synchronizedRanges); EXPECT_EQ(7u, retired[0]);
    pt[1] = 0x9000 | G_PAGE_KERNEL_DEFAULT;
    pagingUnmapPages(0x1000, 2, retired);
    EXPECT_EQ(3u, synchronizedRanges); EXPECT_TRUE(synchronizedGlobal);
}
}
