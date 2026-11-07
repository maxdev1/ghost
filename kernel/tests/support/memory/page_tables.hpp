#pragma once

#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_MASK (G_PAGE_SIZE - 1)
constexpr uintptr_t G_MEM_LOWER_HALF_END = 0x7fffffffffffULL;
#define G_MEM_PHYS_TO_VIRT(p) (p)
uintptr_t currentSpace;
int memoryPhysicalAllocator; bool noMemory;
alignas(4096) unsigned char tablePages[512][4096];
std::atomic<unsigned int> nextPage{0};
uintptr_t bitmapPageAllocatorTryAllocate(int*, uint32_t) {
    if(noMemory) return 0;
    auto page = nextPage++;
    EXPECT_TRUE(page < 512);
    std::memset(tablePages[page], 0xa5, 4096);
    std::this_thread::yield();
    return reinterpret_cast<uintptr_t>(tablePages[page]);
}
void bitmapPageAllocatorMarkFree(int*, uintptr_t) {}
void pagingSynchronizePage(uintptr_t) {}
unsigned synchronizedRanges, synchronizedPages;
uintptr_t synchronizedAddress;
bool synchronizedGlobal;
void pagingSynchronizePages(uintptr_t address, uint32_t pages, bool global = false) {
    synchronizedAddress = address; synchronizedPages = pages; ++synchronizedRanges;
    synchronizedGlobal = global;
}

#include "production/paging_host_hpp.inc"
#include "production/paging_host_cpp.inc"

