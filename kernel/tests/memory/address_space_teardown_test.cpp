#include "support/fixture.hpp"
#include <algorithm>

namespace
{
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_PRESENT 1
#define G_PAGE_LARGE_PAGE_FLAG (1ULL << 7)
#define G_PAGE_UNMANAGED_FLAG (1ULL << 9)
#define G_PAGE_PHYSICAL_MASK 0x000FFFFFFFFFF000ULL
#define G_MEM_PHYS_TO_VIRT(p) (p)
int memoryPhysicalAllocator;
std::vector<uintptr_t> released, reclaimedTables;
uint32_t pageReferenceTrackerDecrement(uintptr_t);
void memoryPhysicalFree(uintptr_t page) {
    if(pageReferenceTrackerDecrement(page) == 0) released.push_back(page);
}
void bitmapPageAllocatorMarkFree(int*, uintptr_t page) {
    EXPECT_EQ(reclaimedTables.end(), std::find(reclaimedTables.begin(), reclaimedTables.end(), page));
    reclaimedTables.push_back(page);
}
uintptr_t pagingGetCurrentSpace() { return 0xdead000; }

#include "production/page_reference_tracker_hpp.inc"
#include "production/page_reference_tracker_cpp.inc"
#include "production/tasking_page_space_teardown.inc"

TEST(AddressSpaces, ReclaimsOwnedFramesAfterLastShareAndPreservesDeviceAndKernelMappings) {
    pageReferenceTrackerInitialize();
    alignas(4096) static uint64_t roots[2][512], tables[2][3][512], kernelTable[512];
    constexpr uintptr_t shared = 0x100000000ULL, privatePage = 0x200000000ULL;
    constexpr uintptr_t mmio = 0xfd000000ULL;
    for(int round = 0; round < 100; ++round) {
        released.clear(); reclaimedTables.clear();
        std::memset(roots, 0, sizeof(roots));
        std::memset(tables, 0, sizeof(tables));
        kernelTable[0] = 0xabc003;
        for(int i = 0; i < 2; ++i) {
            roots[i][255] = reinterpret_cast<uintptr_t>(tables[i][0]) | 7;
            roots[i][256] = reinterpret_cast<uintptr_t>(kernelTable) | 3;
            tables[i][0][17] = reinterpret_cast<uintptr_t>(tables[i][1]) | 7;
            tables[i][1][29] = reinterpret_cast<uintptr_t>(tables[i][2]) | 7;
            tables[i][2][8] = shared | 7 | (1ULL << 63);
            tables[i][2][9] = mmio | 7 | G_PAGE_UNMANAGED_FLAG;
            tables[i][2][10] = 0x12345000; // non-present entries own nothing
            pageReferenceTrackerIncrement(reinterpret_cast<uintptr_t>(roots[i]));
            pageReferenceTrackerIncrement(shared);
        }
        tables[0][2][11] = privatePage | 7;
        pageReferenceTrackerIncrement(privatePage);
        taskingMemoryDestroyPageSpace(reinterpret_cast<uintptr_t>(roots[0]));
        EXPECT_EQ(1, std::count(released.begin(), released.end(), privatePage));
        EXPECT_EQ(0, std::count(released.begin(), released.end(), shared));
        taskingMemoryDestroyPageSpace(reinterpret_cast<uintptr_t>(roots[1]));
        EXPECT_EQ(1, std::count(released.begin(), released.end(), shared));
        EXPECT_TRUE(released.size() == 4 && reclaimedTables.size() == 6);
        EXPECT_EQ(0, std::count(released.begin(), released.end(), mmio));
        EXPECT_TRUE(roots[0][255] == 0 && roots[1][255] == 0);
        EXPECT_TRUE(kernelTable[0] == 0xabc003 && roots[0][256] && roots[1][256]);
    }
    for(auto& bucket : directory.tables)
        while(bucket) { auto next = bucket->next; delete bucket; bucket = next; }
}
}
