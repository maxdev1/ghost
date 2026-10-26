#pragma once

#include "support/synchronization.hpp"

#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_DOWN(p) ((p) & ~(G_PAGE_SIZE - 1))
#define G_MEM_LOWER_END 0x100000
#define LIMINE_MEMMAP_USABLE 0
constexpr uintptr_t physicalBase = UINT64_C(0x100000000);
alignas(4096) uint8_t physicalArena[160 * 1024 * 1024];
#define G_MEM_PHYS_TO_VIRT(p) (reinterpret_cast<uintptr_t>(physicalArena) + (p) - physicalBase)
struct limine_memmap_entry { uint64_t base, length, type; };
struct limine_memmap_response { uint64_t entry_count; limine_memmap_entry** entries; };

#include "production/bitmap_page_allocator_hpp.inc"
#include "production/bitmap_page_allocator_cpp.inc"

class BitmapPageAllocator : public testing::Test
{
protected:
    g_bitmap_page_allocator allocator{};
    std::vector<limine_memmap_entry> regions;
    std::vector<limine_memmap_entry*> regionPointers;

    void initialize(std::initializer_list<limine_memmap_entry> entries)
    {
        regions = entries;
        regionPointers.clear();
        for(auto& region : regions) regionPointers.push_back(&region);
        limine_memmap_response map{regions.size(), regionPointers.data()};
        bitmapPageAllocatorInitialize(&allocator, &map);
    }

    void initializePages(uint32_t pages)
    {
        initialize({{physicalBase, pages * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE}});
    }

    std::set<uintptr_t> expectedPages(uint32_t totalPages)
    {
        // These small fixtures reserve their first page for the bitmap and last
        // page for the bootstrap index. Derive the oracle from input, not bits.
        std::set<uintptr_t> result;
        for(uint32_t page = 1; page + 1 < totalPages; ++page)
            result.insert(physicalBase + page * G_PAGE_SIZE);
        return result;
    }

    std::vector<uintptr_t> drain()
    {
        std::vector<uintptr_t> result;
        // Bound the loop even if a regression creates phantom free-page tickets.
        const auto budget = allocator.freePageCount;
        for(uint64_t i = 0; i <= budget; ++i)
        {
            auto page = bitmapPageAllocatorTryAllocate(&allocator, 0);
            if(!page) return result;
            result.push_back(page);
        }
        ADD_FAILURE() << "allocator returned more frames than its free-page budget";
        return result;
    }

    void expectUniquePages(const std::vector<uintptr_t>& pages, const std::set<uintptr_t>& expected)
    {
        const std::set<uintptr_t> actual(pages.begin(), pages.end());
        EXPECT_EQ(pages.size(), actual.size()) << "a frame was allocated twice";
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(0U, allocator.freePageCount);
        EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 0));
    }

    void TearDown() override
    {
        EXPECT_EQ(0, allocator.lock.depth);
        EXPECT_EQ(0, allocator.fastBuffer.lock.depth);
        if(allocator.indexPage)
            for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES && allocator.indexPage->entries[i]; ++i)
                EXPECT_EQ(0, allocator.indexPage->entries[i]->lock.depth);
    }
};
