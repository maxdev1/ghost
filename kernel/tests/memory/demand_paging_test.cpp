#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_DOWN(p) ((p) & ~(G_PAGE_SIZE - 1))
#define G_PAGE_ALIGN_UP(p) (((p) + G_PAGE_SIZE - 1) & ~(G_PAGE_SIZE - 1))
#define G_MEM_PHYS_TO_VIRT(p) (p)
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
using g_offset = uintptr_t;
struct g_task {};
struct mapping { uintptr_t fileStart, fileSize, memSize, fileOffset; int fd; };
mapping demandMapping{0x400123, 0x100, 0x300, 17, 1};
alignas(4096) unsigned char demandPage[4096];
uintptr_t mapped;
int allocations, freed;
bool failRead, competingPublisher;
mapping* memoryOnDemandFindMapping(g_task*, uintptr_t) { return &demandMapping; }
uintptr_t memoryPhysicalTryAllocate() { ++allocations; return reinterpret_cast<uintptr_t>(demandPage); }
void memoryPhysicalFree(uintptr_t p) { EXPECT_EQ(reinterpret_cast<uintptr_t>(demandPage), p); ++freed; }
uintptr_t pagingVirtualToPhysical(uintptr_t) { return mapped; }
void pagingInvalidatePage(uintptr_t) {}
#define G_PAGE_PRESENT 1
uintptr_t pagingVirtualToPageEntry(uintptr_t) { return mapped ? mapped | 1 : 0; }
bool pagingMapPage(uintptr_t, uintptr_t physical, int, int) {
    if(competingPublisher) { mapped = 0x2000; return false; }
    if(mapped) return false;
    mapped = physical;
    return true;
}
void* memorySetBytes(void* p, uint8_t value, int32_t size) {
    if(reinterpret_cast<uintptr_t>(p) >= 0x400000 && reinterpret_cast<uintptr_t>(p) < 0x401000)
        p = demandPage + (reinterpret_cast<uintptr_t>(p) - 0x400000);
    return std::memset(p, value, size);
}
bool filesystemReadToMemory(int, uintptr_t offset, uint8_t* target, uintptr_t size) {
    EXPECT_FALSE(mapped); // Other CPUs must not see the page until file I/O is complete.
    EXPECT_TRUE(offset == 17 && target == demandPage + 0x123 && size == 0x100);
    if(failRead) return false;
    std::memset(target, 0x5a, size);
    return true;
}

#include "production/memoryOnDemandHandlePageFault.inc"

TEST(DemandPaging, InitializesPagesBeforePublicationAndReclaimsFailedAllocations) {
    g_task task;
    EXPECT_TRUE(memoryOnDemandHandlePageFault(&task, 0x400123));
    EXPECT_EQ(reinterpret_cast<uintptr_t>(demandPage), mapped);
    for(int i = 0; i < 4096; ++i)
        EXPECT_EQ((i >= 0x123 && i < 0x223 ? 0x5a : 0), demandPage[i]);
    // A second fault for the same already-published page needs no replacement.
    EXPECT_TRUE(memoryOnDemandHandlePageFault(&task, 0x400123));
    EXPECT_EQ(1, allocations);
    mapped = 0;
    failRead = true;
    EXPECT_FALSE(memoryOnDemandHandlePageFault(&task, 0x400123));
    EXPECT_TRUE(!mapped && freed == 1);
    failRead = false;
    competingPublisher = true;
    EXPECT_TRUE(memoryOnDemandHandlePageFault(&task, 0x400123));
    EXPECT_TRUE(mapped == 0x2000 && freed == 2);
}
}
