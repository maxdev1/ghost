#include "support/fixture.hpp"
#include <initializer_list>
#include <map>

namespace
{
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_UP(v) (((v) + 4095ULL) & ~4095ULL)
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
#define G_SECURITY_LEVEL_DRIVER 1
struct g_address_range_pool {} pool;
struct g_process { g_mutex lock; g_address_range_pool* virtualRangePool; } process{{}, &pool};
struct g_task { g_process* process; int securityLevel; };
struct g_syscall_alloc_mem { uint64_t size; void* virtualResult; void* physicalResult; };
std::map<uintptr_t, uintptr_t> entries;
constexpr uintptr_t base = 0x100000;
int allocated, released, zeroed, attempts, ranges, failPhysical = -1, failMapping = -1;
uintptr_t addressRangePoolAllocate(g_address_range_pool*, uint32_t pages) {
    EXPECT_TRUE(process.lock.depth > 0 && pages); ++ranges; return base;
}
void addressRangePoolFree(g_address_range_pool*, uintptr_t address) { EXPECT_EQ(base, address); --ranges; }
uintptr_t memoryPhysicalTryAllocate() { return allocated == failPhysical ? 0 : ++allocated * 4096; }
void memoryPhysicalFree(uintptr_t page) {
    for(auto& entry : entries) EXPECT_NE(page, entry.second); ++released;
}
bool pagingMapPage(uintptr_t address, uintptr_t page, int, int) {
    if(attempts++ == failMapping) { entries[address] = 0xabc000; return false; }
    entries[address] = page; return true;
}
uintptr_t pagingVirtualToPhysical(uintptr_t address) { return entries.at(address); }
void pagingUnmapPage(uintptr_t address) { EXPECT_EQ(1, entries.erase(address)); }
void memorySetBytes(void* address, int value, size_t size) {
    EXPECT_TRUE(value == 0 && size == 4096 && (uintptr_t)address == allocated * 4096); ++zeroed;
}
#define G_MEM_PHYS_TO_VIRT(p) (p)
#include "production/memorySizeToPageCount.inc"
#include "production/syscallAllocateMemory.inc"

void reset() { entries.clear(); allocated = released = zeroed = attempts = ranges = 0; failPhysical = failMapping = -1; }
TEST(AllocateMemory, RollsBackPartialMappingsBeforeReleasingFrames) {
    g_task task{&process, 1};
    for(int fail = 0; fail < 4; ++fail) {
        reset(); failPhysical = fail;
        g_syscall_alloc_mem call{4 * 4096, (void*)1, (void*)1};
        syscallAllocateMemory(&task, &call);
        EXPECT_TRUE(!call.virtualResult && !call.physicalResult && entries.empty());
        EXPECT_TRUE(released == allocated && ranges == 0 && process.lock.depth == 0);
        reset(); failMapping = fail;
        syscallAllocateMemory(&task, &call);
        EXPECT_TRUE(!call.virtualResult && !call.physicalResult && released == allocated && ranges == 0);
        EXPECT_TRUE(entries.size() == 1 && entries.begin()->second == 0xabc000); // preserve foreign mapping
    }
    reset();
    g_syscall_alloc_mem call{1, nullptr, nullptr};
    syscallAllocateMemory(&task, &call);
    EXPECT_TRUE(call.virtualResult == (void*)base && call.physicalResult == (void*)4096 && zeroed == 1);
    reset(); task.securityLevel = 3;
    syscallAllocateMemory(&task, &call);
    EXPECT_TRUE(call.virtualResult == (void*)base && !call.physicalResult);
    reset(); call.size = 0;
    syscallAllocateMemory(&task, &call);
    EXPECT_TRUE(!call.virtualResult && !call.physicalResult && !allocated && !ranges);
    for(uint64_t size : std::initializer_list<uint64_t>{UINT64_MAX, (1ULL << 44), UINT64_MAX - 4095}) {
        reset(); call.size = size;
        syscallAllocateMemory(&task, &call);
        EXPECT_TRUE(!call.virtualResult && !call.physicalResult && !allocated && !ranges);
    }
}
}
