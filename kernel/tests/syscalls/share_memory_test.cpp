/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
#define G_PAGE_USER_FLAG 4
#define G_PAGE_WRITABLE_FLAG 2
#define G_PAGE_LARGE_PAGE_FLAG 128
#define G_PAGE_UNMANAGED_FLAG (1ULL << 9)
#define G_PAGE_WRITE_THROUGH (1ULL << 3)
#define G_PAGE_CACHE_DISABLE (1ULL << 4)
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
int failAt = -1, mapAttempts, allocated, freedRanges, unmapBatches;
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
void pagingUnmapPages(uintptr_t virt, uint32_t pages, uint64_t* retired) {
    EXPECT_EQ(2, currentSpace); EXPECT_LE(pages, 256u); ++unmapBatches;
    for(uint32_t page = 0; page < pages; ++page) {
        const auto address = virt + page * G_PAGE_SIZE;
        retired[page] = pagingVirtualToPageEntry(address);
        targetEntries.erase(address);
    }
}

#include "production/memoryReleasePages.inc"

#include "production/syscall_share_and_unmap.inc"

void reset() {
    EXPECT_EQ(0, pins);
    EXPECT_TRUE(heapBlocks.empty());
    sourceEntries.clear(); targetEntries.clear(); references.clear();
    sourceEntries[0x400000] = ram | 7;
    sourceEntries[0x401000] = mmio | 7 | G_PAGE_UNMANAGED_FLAG | G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH;
    references[ram] = 1;
    targetProcess.terminating = false;
    targetRange.used = false;
    failAt = -1; mapAttempts = allocated = freedRanges = unmapBatches = 0;
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
        EXPECT_EQ(0u, targetEntries[targetBase] & (G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH));
        EXPECT_EQ(G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH,
                  targetEntries[targetBase + 4096] & (G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH));
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

TEST(ShareMemory, PreservesEachSourceCachePolicyWhenCreatingAliases) {
    for(uint64_t cacheFlags: {0ULL, G_PAGE_WRITE_THROUGH, G_PAGE_CACHE_DISABLE,
                             G_PAGE_WRITE_THROUGH | G_PAGE_CACHE_DISABLE}) {
        reset();
        sourceEntries[0x400000] |= cacheFlags;
        g_syscall_share_mem share{reinterpret_cast<void*>(0x400000), 4096, 2, nullptr};
        syscallShareMemory(&sourceTask, &share);
        ASSERT_EQ(reinterpret_cast<void*>(targetBase), share.virtualAddress);
        EXPECT_EQ(cacheFlags, targetEntries[targetBase] & (G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH));
        EXPECT_EQ(2, references[ram]);

        currentSpace = 2;
        g_syscall_unmap unmap{targetBase};
        syscallUnmap(&targetTask, &unmap);
        EXPECT_EQ(1, references[ram]);
        EXPECT_TRUE(targetEntries.empty());
    }
}

TEST(ShareMemory, BulkUnmapPreservesSourceReferencesAndDeviceFramesAcrossBatches) {
    reset();
    targetRange = {targetBase, 600, G_PROC_VIRTUAL_RANGE_FLAG_NONE, true};
    for(uint32_t page = 0; page < 600; ++page) {
        if(page == 1) continue; // An unmapped guard/hole is harmless.
        targetEntries[targetBase + page * G_PAGE_SIZE] = page == 2 ?
            mmio | G_PAGE_USER_DEFAULT | G_PAGE_UNMANAGED_FLAG : ram | G_PAGE_USER_DEFAULT;
    }
    references[ram] = 599; // Original source plus 598 managed aliases.
    currentSpace = 2;
    g_syscall_unmap call{targetBase};
    syscallUnmap(&targetTask, &call);
    EXPECT_EQ(3, unmapBatches); EXPECT_EQ(1, references[ram]);
    EXPECT_EQ(0u, references.count(mmio)); EXPECT_TRUE(targetEntries.empty());
    EXPECT_FALSE(targetRange.used); EXPECT_EQ(1, freedRanges);
    EXPECT_EQ(0, targetProcess.lock.depth);
}

TEST(ShareMemory, RollsBackUncachedAliasesWithoutReleasingDeviceFrames) {
    reset();
    const uint64_t deviceFlags = G_PAGE_USER_DEFAULT | G_PAGE_UNMANAGED_FLAG |
                                 G_PAGE_CACHE_DISABLE | G_PAGE_WRITE_THROUGH;
    sourceEntries[0x402000] = (mmio + 4096) | deviceFlags;
    targetEntries[targetBase + 8192] = 0x300000007ULL;
    failAt = 2;
    g_syscall_share_mem share{reinterpret_cast<void*>(0x400000), 12288, 2, nullptr};
    syscallShareMemory(&sourceTask, &share);

    EXPECT_EQ(nullptr, share.virtualAddress);
    EXPECT_EQ(1u, targetEntries.size());
    EXPECT_EQ(0x300000007ULL, targetEntries[targetBase + 8192]);
    EXPECT_EQ(mmio | deviceFlags, sourceEntries[0x401000]);
    EXPECT_EQ(1, references[ram]);
    EXPECT_EQ(0u, references.count(mmio));
    EXPECT_EQ(0u, references.count(mmio + 4096));
    EXPECT_EQ(1, freedRanges);
    EXPECT_EQ(1u, currentSpace);
    EXPECT_EQ(0, pins);
    EXPECT_TRUE(heapBlocks.empty());
}

TEST(ShareMemory, RejectsSupervisorReadOnlyAndLargeSourcePages) {
    for(uint64_t flags : {uint64_t(G_PAGE_PRESENT), uint64_t(G_PAGE_PRESENT | G_PAGE_USER_FLAG),
                          uint64_t(G_PAGE_USER_DEFAULT | G_PAGE_LARGE_PAGE_FLAG)}) {
        reset();
        sourceEntries[0x400000] = ram | flags;
        g_syscall_share_mem share{(void*) 0x400000, 4096, 2, nullptr};
        syscallShareMemory(&sourceTask, &share);
        EXPECT_EQ(nullptr, share.virtualAddress);
        EXPECT_EQ(1, references[ram]);
        EXPECT_TRUE(targetEntries.empty());
        EXPECT_EQ(0, pins);
    }
}

}
