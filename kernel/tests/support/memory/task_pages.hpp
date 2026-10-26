#pragma once

#include <map>
#include <unordered_set>
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_MASK 4095ULL
#define G_PAGE_ALIGN_DOWN(v) ((v) & ~4095ULL)
#define G_PAGE_ALIGN_UP(v) (((v) + 4095ULL) & ~4095ULL)
#define G_ADDRESS_MAX UINTPTR_MAX
#define G_MEM_PHYS_TO_VIRT(p) (p)
#define G_PAGE_PRESENT 1
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
#define G_PAGE_TABLE_KERNEL_DEFAULT 3
#define G_PAGE_KERNEL_DEFAULT 3
#define G_SECURITY_LEVEL_KERNEL 0
#define G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL 2
using g_address_range_pool = int;
struct g_stack { uintptr_t start, end; };
struct g_process {
    g_mutex lock;
    struct { uintptr_t start, end; } image;
    struct { uintptr_t brk, start; uint32_t pages; } heap;
    uintptr_t pageSpace;
    struct { uintptr_t location; uint32_t size, userThreadOffset; } tlsMaster;
    g_address_range_pool* virtualRangePool;
};
struct g_kernel_threadlocal { uint32_t processor; };
struct g_user_threadlocal { g_user_threadlocal* self; };
struct g_task {
    g_process* process; g_stack stack; int securityLevel, id;
    struct { g_kernel_threadlocal* kernelThreadLocal; g_user_threadlocal* userThreadLocal; uintptr_t start, end; } threadLocal;
};
alignas(4096) uint8_t arena[16 * 4096];
#define G_USER_MAXIMUM_HEAP_BREAK ((uintptr_t)arena + 8 * G_PAGE_SIZE)
std::map<uintptr_t, uintptr_t> mappings;
std::unordered_set<uintptr_t> frames;
uintptr_t currentSpace = 0x1000;
int allocations, frees, mapAttempts, invalidations, ranges;
int failPhysical = -1, failMapping = -1;
bool noRange;
uintptr_t memoryPhysicalTryAllocate() {
    if(allocations == failPhysical) return 0;
    void* p = nullptr; EXPECT_EQ(0, posix_memalign(&p, 4096, 4096));
    std::memset(p, 0xa5, 4096); frames.insert((uintptr_t)p); ++allocations;
    return (uintptr_t)p;
}
void memoryPhysicalFree(uintptr_t p) {
    for(auto entry : mappings) EXPECT_NE(p, entry.second);
    EXPECT_EQ(1, frames.erase(p)); std::free((void*)p); ++frees;
}
void memorySetBytes(void* p, int v, size_t n) { std::memset(p, v, n); }
bool pagingMapPage(uintptr_t v, uintptr_t p, uint32_t, uint32_t) {
    for(size_t i = 0; i < 4096; ++i) EXPECT_EQ(0, ((uint8_t*)p)[i]);
    if(mapAttempts++ == failMapping) { mappings[v] = 0xbeef000; return false; }
    if(mappings.count(v)) return false;
    mappings[v] = p; return true;
}
uintptr_t pagingVirtualToPhysical(uintptr_t v) { return mappings.at(v); }
uintptr_t pagingVirtualToPageEntry(uintptr_t v) {
    auto i = mappings.find(v); return i == mappings.end() ? 0 : i->second | 1;
}
void pagingUnmapPage(uintptr_t v) { EXPECT_EQ(1, mappings.erase(v)); }
void pagingInvalidatePage(uintptr_t) { ++invalidations; }
uintptr_t taskingMemoryTemporarySwitchTo(uintptr_t p) {
    auto old = currentSpace; currentSpace = p; return old;
}
void taskingMemoryTemporarySwitchBack(uintptr_t p) { currentSpace = p; }
uintptr_t addressRangePoolAllocate(int*, uint32_t pages, uint8_t flags) {
    EXPECT_EQ(G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL, flags);
    EXPECT_TRUE(pages); if(noRange) return 0; ++ranges; return (uintptr_t)arena;
}
void addressRangePoolFree(int*, uintptr_t p) { EXPECT_EQ((uintptr_t)arena, p); --ranges; }
void reset() {
    EXPECT_TRUE(frames.empty()); mappings.clear(); allocations = frees = mapAttempts = invalidations = ranges = 0;
    failPhysical = failMapping = -1; noRange = false;
}
