#include "support/fixture.hpp"

namespace
{
#include "support/synchronization.hpp"

#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_DOWN(p) ((p) & ~(G_PAGE_SIZE - 1))
#define G_MEM_LOWER_END 0x100000
#define LIMINE_MEMMAP_USABLE 0
constexpr uintptr_t physicalBase = 0x100000000ULL;
alignas(4096) unsigned char physicalArena[300 * G_PAGE_SIZE];
#define G_MEM_PHYS_TO_VIRT(p) (reinterpret_cast<uintptr_t>(physicalArena) + (p) - physicalBase)
struct limine_memmap_entry { uint64_t base, length, type; };
struct limine_memmap_response { uint64_t entry_count; limine_memmap_entry** entries; };

#include "production/bitmap_page_allocator_hpp.inc"
#include "production/bitmap_page_allocator_cpp.inc"

TEST(BitmapPageAllocator, PreservesReserveUnderConcurrentExhaustionAndRecoversFreedPages) {
    g_bitmap_page_allocator allocator;
    limine_memmap_entry entry{physicalBase, sizeof(physicalArena), LIMINE_MEMMAP_USABLE};
    limine_memmap_entry* entries[] = {&entry}; limine_memmap_response map{1, entries};
    bitmapPageAllocatorInitialize(&allocator, &map);
    const auto usable = allocator.freePageCount;
    for(int round = 0; round < 10; ++round) {
        std::vector<uintptr_t> claimed; std::mutex resultLock;
        std::vector<std::thread> threads;
        for(int i = 0; i < 8; ++i) threads.emplace_back([&] {
            while(auto p = bitmapPageAllocatorTryAllocate(&allocator, 8)) {
                std::lock_guard<std::mutex> guard(resultLock);
                for(auto other : claimed) EXPECT_NE(p, other);
                claimed.push_back(p);
            }
        });
        for(auto& t : threads) t.join();
        EXPECT_TRUE(allocator.freePageCount == 8 && claimed.size() == usable - 8);
        for(int i = 0; i < 8; ++i) claimed.push_back(bitmapPageAllocatorAllocate(&allocator));
        EXPECT_TRUE(!bitmapPageAllocatorTryAllocate(&allocator, 0) && allocator.freePageCount == 0);
        for(auto p : claimed) bitmapPageAllocatorMarkFree(&allocator, p);
        EXPECT_EQ(usable, allocator.freePageCount);
    }
}
}
