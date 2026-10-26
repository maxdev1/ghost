#include "support/fixture.hpp"
#include <map>

namespace
{
#include "support/tracked_heap.hpp"

bool failHeap;
void* heapTryAllocate(uint32_t size) { return failHeap ? nullptr : heapAllocate(size); }
bool memorySizeToPageCount(uint64_t size, uint32_t* pages) {
    if(!size || size > (uint64_t)UINT32_MAX * 4096) return false;
    *pages = (size + 4095) / 4096; return true;
}
#define G_USER_VIRTUAL_RANGES_START 0xa0000000ULL
#define G_USER_VIRTUAL_RANGES_END 0xc0000000ULL
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_MASK (G_PAGE_SIZE - 1)
#define G_PAGE_ALIGN_UP(p) (((p) + 4095) & ~4095ULL)
#define G_PAGE_PRESENT 1
#define G_PAGE_UNMANAGED_FLAG (1ULL << 9)
#define G_PAGE_PHYSICAL_MASK 0x000FFFFFFFFFF000ULL
#define G_MEM_LOWER_HALF_END 0x7fffffffffffULL
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE 0
#define G_PROC_VIRTUAL_RANGE_FLAG_WEAK 1
#define G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL 2
using g_tid = int;
struct g_address_range { uintptr_t base; uint32_t pages; uint8_t flags; bool used; } targetRange;
struct g_address_range_pool {} sourcePool, targetPool;
struct g_process { g_mutex lock; g_address_range_pool* virtualRangePool; uintptr_t pageSpace; bool terminating; };
g_process sourceProcess{{}, &sourcePool, 1, false}, targetProcess{{}, &targetPool, 2, false};
struct g_task { g_process* process; } sourceTask{&sourceProcess}, targetTask{&targetProcess};
struct g_syscall_share_mem { void* memory; uint32_t size; int processId; void* virtualAddress; };
struct g_syscall_unmap { uintptr_t virtualBase; };
int pins;
g_task* taskingAcquireTask(int id) { EXPECT_EQ(2, id); ++pins; return &targetTask; }
void taskingReleaseTask(g_task* task) { if(task) --pins; }
constexpr uintptr_t ram = 0x100000000ULL, mmio = 0xfd000000ULL, targetBase = 0xa0000000;
uintptr_t currentSpace = 1;
int failAt = -1, mapAttempts, allocated, freedRanges;
std::map<uintptr_t, uint64_t> sourceEntries, targetEntries;
std::map<uintptr_t, int> references;
uint64_t pagingVirtualToPageEntry(uintptr_t virt) {
    auto& entries = currentSpace == 1 ? sourceEntries : targetEntries;
    auto it = entries.find(virt);
    return it == entries.end() ? 0 : it->second;
}
void pageReferenceTrackerIncrement(uintptr_t physical) {
    EXPECT_TRUE(sourceProcess.lock.depth > 0 && physical == ram && references[ram] > 0);
    ++references[physical];
}
void memoryPhysicalFree(uintptr_t physical) {
    EXPECT_TRUE(physical == ram && references[physical] > 1);
    --references[physical];
}
uintptr_t addressRangePoolAllocate(g_address_range_pool* pool, uint32_t pages, uint8_t flags) {
    EXPECT_TRUE(pool == &targetPool && targetProcess.lock.depth > 0);
    targetRange = {targetBase, pages, flags, true};
    ++allocated;
    return targetBase;
}
g_address_range* addressRangePoolFind(g_address_range_pool*, uintptr_t base) {
    EXPECT_EQ(targetBase, base); return &targetRange;
}
void addressRangePoolFree(g_address_range_pool*, uintptr_t base) {
    EXPECT_TRUE(base == targetBase && targetProcess.lock.depth > 0);
    targetRange.used = false; ++freedRanges;
}
uintptr_t taskingMemoryTemporarySwitchTo(uintptr_t space) {
    auto back = currentSpace; currentSpace = space; return back;
}
void taskingMemoryTemporarySwitchBack(uintptr_t space) { currentSpace = space; }
bool pagingMapPage(uintptr_t virt, uintptr_t physical, uint64_t, uint64_t flags) {
    EXPECT_TRUE(currentSpace == 2 && targetProcess.lock.depth > 0);
    if(physical == ram) EXPECT_EQ(2, references[ram]); // retain precedes publication
    if(mapAttempts++ == failAt) return false;
    targetEntries[virt] = physical | flags; return true;
}
void pagingUnmapPage(uintptr_t virt) { EXPECT_EQ(2, currentSpace); targetEntries.erase(virt); }

#include "production/syscall_share_and_unmap.inc"

void reset() {
    EXPECT_EQ(0, pins);
    EXPECT_TRUE(heapBlocks.empty());
    sourceEntries.clear(); targetEntries.clear(); references.clear();
    sourceEntries[0x400000] = ram | 7;
    sourceEntries[0x401000] = mmio | 7 | G_PAGE_UNMANAGED_FLAG;
    references[ram] = 1;
    targetProcess.terminating = false;
    targetRange.used = false;
    failAt = -1; mapAttempts = allocated = freedRanges = 0;
    currentSpace = 1;
}
TEST(ShareMemory, RetainsRamPreservesDeviceOwnershipAndRollsBackFailures) {
    reset();
    for(uint32_t size : {0U, 0x20000001U, UINT32_MAX}) {
        g_syscall_share_mem invalid{reinterpret_cast<void*>(0x400000), size, 2, nullptr};
        syscallShareMemory(&sourceTask, &invalid);
        EXPECT_TRUE(!invalid.virtualAddress && heapBlocks.empty() && !allocated);
    }
    failHeap = true;
    g_syscall_share_mem oom{reinterpret_cast<void*>(0x400000), 4096, 2, nullptr};
    syscallShareMemory(&sourceTask, &oom);
    EXPECT_TRUE(!oom.virtualAddress && references[ram] == 1 && !allocated);
    EXPECT_EQ(0, pins);
    failHeap = false;
    for(int round = 0; round < 100; ++round) {
        reset();
        g_syscall_share_mem share{reinterpret_cast<void*>(0x400000), 8192, 2, nullptr};
        syscallShareMemory(&sourceTask, &share);
        EXPECT_EQ(reinterpret_cast<void*>(targetBase), share.virtualAddress);
        EXPECT_TRUE(references[ram] == 2 && targetEntries.size() == 2);
        EXPECT_FALSE((targetEntries[targetBase] & G_PAGE_UNMANAGED_FLAG));
        EXPECT_TRUE(targetEntries[targetBase + 4096] & G_PAGE_UNMANAGED_FLAG);
        EXPECT_TRUE(heapBlocks.empty());
        currentSpace = 2;
        g_syscall_unmap unmap{targetBase};
        syscallUnmap(&targetTask, &unmap);
        EXPECT_TRUE(references[ram] == 1 && targetEntries.empty());
        EXPECT_TRUE(references.count(mmio) == 0 && freedRanges == 1);
    }
    // Sharing only MMIO must preserve non-owning semantics in both PTE and range.
    reset();
    g_syscall_share_mem device{reinterpret_cast<void*>(0x401000), 4096, 2, nullptr};
    syscallShareMemory(&sourceTask, &device);
    EXPECT_TRUE(targetRange.flags == G_PROC_VIRTUAL_RANGE_FLAG_WEAK && references.count(mmio) == 0);
    currentSpace = 2;
    g_syscall_unmap unmap{targetBase};
    syscallUnmap(&targetTask, &unmap);
    EXPECT_TRUE(references[ram] == 1 && targetEntries.empty());

    // Stacks/TLS keep kernel references and cannot be released through public unmap.
    reset();
    g_syscall_share_mem internal{reinterpret_cast<void*>(0x400000), 4096, 2, nullptr};
    syscallShareMemory(&sourceTask, &internal);
    targetRange.flags |= G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL;
    currentSpace = 2;
    syscallUnmap(&targetTask, &unmap);
    EXPECT_TRUE(targetRange.used && targetEntries.size() == 1 && references[ram] == 2 && !freedRanges);

    // A source hole, dying receiver, or failed target publication drops retained refs.
    reset(); sourceEntries.erase(0x401000);
    g_syscall_share_mem hole{reinterpret_cast<void*>(0x400000), 8192, 2, nullptr};
    syscallShareMemory(&sourceTask, &hole);
    EXPECT_TRUE(!hole.virtualAddress && references[ram] == 1 && !allocated && heapBlocks.empty());
    reset(); targetProcess.terminating = true;
    syscallShareMemory(&sourceTask, &hole);
    EXPECT_TRUE(!hole.virtualAddress && references[ram] == 1 && !allocated && heapBlocks.empty());
    reset(); failAt = 1;
    targetEntries[targetBase + 4096] = 0x300000007ULL;
    syscallShareMemory(&sourceTask, &hole);
    EXPECT_TRUE(!hole.virtualAddress && references[ram] == 1 && heapBlocks.empty());
    EXPECT_TRUE(targetEntries.size() == 1 && targetEntries[targetBase + 4096] == 0x300000007ULL);
    EXPECT_TRUE(freedRanges == 1 && currentSpace == 1);
    EXPECT_EQ(0, pins);
}
}
