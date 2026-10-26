#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_DOWN(p) ((p) & ~(G_PAGE_SIZE - 1))
#define G_MEM_LOWER_END 0x100000
#define LIMINE_MEMMAP_USABLE 0
constexpr uintptr_t physicalBase = 0x100000000ULL;
alignas(4096) unsigned char physicalArena[160 * 1024 * 1024];
#define G_MEM_PHYS_TO_VIRT(p) (reinterpret_cast<uintptr_t>(physicalArena) + (p) - physicalBase)
struct limine_memmap_entry { uint64_t base, length, type; };
struct limine_memmap_response { uint64_t entry_count; limine_memmap_entry** entries; };

#include "production/bitmap_page_allocator_hpp.inc"
#include "production/bitmap_page_allocator_cpp.inc"

TEST(BitmapPageAllocator, ReusesEveryPageInPartialRegionsAboveFourGiB)
{
    g_bitmap_page_allocator allocator;
    limine_memmap_entry entry{physicalBase, 300 * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE};
    limine_memmap_entry* entries[] = {&entry};
    limine_memmap_response map{1, entries};
    bitmapPageAllocatorInitialize(&allocator, &map);
    // One index page and one bitmap page are reserved. All addresses exceed 4 GiB.
    const unsigned int usable = 298;
    EXPECT_EQ(usable, allocator.freePageCount);
    std::vector<uintptr_t> allocated;
    for(unsigned int i = 0; i < usable; ++i) {
        auto page = bitmapPageAllocatorAllocate(&allocator);
        EXPECT_TRUE(page >= entry.base + G_PAGE_SIZE && page < entry.base + entry.length);
        allocated.push_back(page);
        EXPECT_EQ(usable - i - 1, allocator.freePageCount);
    }
    // Free more than the fast buffer can hold, then reclaim both buffer and bitmap pages.
    for(auto page : allocated) bitmapPageAllocatorMarkFree(&allocator, page);
    EXPECT_EQ(usable, allocator.freePageCount);
    for(unsigned int i = 0; i < usable; ++i) {
        auto page = bitmapPageAllocatorAllocate(&allocator);
        bool found = false;
        for(auto& old : allocated) if(old == page) { old = 0; found = true; break; }
        EXPECT_TRUE(found && allocator.freePageCount == usable - i - 1);
    }
}

TEST(BitmapPageAllocator, AccountsForAll64BitsOfLargeRegionBitmaps)
{
    g_bitmap_page_allocator allocator;
    limine_memmap_entry entry{};
    limine_memmap_entry* entries[] = {&entry};
    limine_memmap_response map{1, entries};
    // A long region must use every bit of a 64-bit bitmap entry.
    entry = {physicalBase, 160 * 1024 * 1024ULL, LIMINE_MEMMAP_USABLE};
    bitmapPageAllocatorInitialize(&allocator, &map);
    size_t bitmapCount = 0;
    for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES; ++i) {
        auto bitmap = allocator.indexPage->entries[i];
        if(!bitmap) break;
        ++bitmapCount;
        EXPECT_TRUE(bitmap->end <= entry.base + entry.length);
    }
    EXPECT_EQ(2, bitmapCount);
    EXPECT_EQ((160 * 1024 * 1024 / G_PAGE_SIZE) - bitmapCount - 1, allocator.freePageCount);
}

TEST(BitmapPageAllocator, StopsPublishingRegionsAtIndexCapacity)
{
    g_bitmap_page_allocator allocator;
    // Exhaust the single index with many small firmware memory-map regions.
    limine_memmap_entry fragments[G_BITMAP_INDEX_MAX_ENTRIES + 2];
    limine_memmap_entry* fragmentPointers[G_BITMAP_INDEX_MAX_ENTRIES + 2];
    for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES + 2; ++i) {
        fragments[i] = {physicalBase + i * 3 * G_PAGE_SIZE, 3 * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE};
        fragmentPointers[i] = &fragments[i];
    }
    limine_memmap_response map{G_BITMAP_INDEX_MAX_ENTRIES + 2, fragmentPointers};
    bitmapPageAllocatorInitialize(&allocator, &map);
    EXPECT_EQ(G_BITMAP_INDEX_MAX_ENTRIES * 2 - 1, allocator.freePageCount);
}

}
