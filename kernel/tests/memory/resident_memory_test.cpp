#include "support/fixture.hpp"

namespace
{
#include "support/memory/page_tables.hpp"

TEST(ResidentMemory, CountsOnlyPresentManagedUserLeavesInTheLowerHalf)
{
	alignas(4096) uint64_t root[512]{}, pdpt[512]{}, pd[512]{}, pt[512]{};
	root[0] = (uintptr_t)pdpt | 7;
	root[256] = (uintptr_t)pdpt | 7;
	pdpt[0] = (uintptr_t)pd | 7;
	pd[0] = (uintptr_t)pt | 7;
	pt[0] = 0x1000 | 7;
	pt[1] = 0x2000 | 7;
	pt[2] = 0x3000 | G_PAGE_USER_FLAG;
	pt[3] = 0x4000 | G_PAGE_PRESENT;
	pt[4] = 0x5000 | 7 | G_PAGE_UNMANAGED_FLAG;
	EXPECT_EQ(8192U, pagingCountResidentUserBytes((uintptr_t)root));
	pt[1] = 0;
	EXPECT_EQ(4096U, pagingCountResidentUserBytes((uintptr_t)root));
	root[0] &= ~G_PAGE_USER_FLAG;
	EXPECT_EQ(0U, pagingCountResidentUserBytes((uintptr_t)root));
	EXPECT_EQ(0U, pagingCountResidentUserBytes(0));
}

TEST(ResidentMemory, CountsHugeLeavesWithoutFollowingThemAsTables)
{
	alignas(4096) uint64_t root[512]{}, pdpt[512]{}, pd[512]{};
	root[0] = (uintptr_t)pdpt | 7 | G_PAGE_NX_FLAG;
	pdpt[0] = 0x80000000ULL | 7 | G_PAGE_LARGE_PAGE_FLAG;
	pdpt[1] = (uintptr_t)pd | 7;
	pd[0] = 0x40000000ULL | 7 | G_PAGE_LARGE_PAGE_FLAG;
	pd[1] = 0x50000000ULL | 7 | G_PAGE_LARGE_PAGE_FLAG | G_PAGE_UNMANAGED_FLAG;
	EXPECT_EQ((1ULL << 30) + (1ULL << 21), pagingCountResidentUserBytes((uintptr_t)root));
}
}
