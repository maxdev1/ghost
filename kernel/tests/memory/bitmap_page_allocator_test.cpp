#include "support/fixture.hpp"
#include <set>
#include <initializer_list>

namespace
{
#include "support/memory/bitmap_page_allocator.hpp"

TEST_F(BitmapPageAllocator, ExcludesBitmapAndIndexMetadataFromAllocations)
{
    initializePages(12);
    EXPECT_EQ(10U, allocator.freePageCount);
    EXPECT_EQ(0, allocator.fastBuffer.size);
    expectUniquePages(drain(), expectedPages(12));
}

TEST_F(BitmapPageAllocator, SkipsReservedAndSubPageMemoryRegions)
{
    std::memset(physicalArena, 0xa5, G_PAGE_SIZE);
    initialize({{physicalBase, 32 * G_PAGE_SIZE, 1},
                {physicalBase + 40 * G_PAGE_SIZE, G_PAGE_SIZE - 1, LIMINE_MEMMAP_USABLE},
                {physicalBase + 64 * G_PAGE_SIZE, 10 * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE}});
    std::set<uintptr_t> expected;
    for(unsigned page = 65; page < 73; ++page) expected.insert(physicalBase + page * G_PAGE_SIZE);
    expectUniquePages(drain(), expected);
    for(unsigned byte = 0; byte < G_PAGE_SIZE; ++byte) EXPECT_EQ(0xa5, physicalArena[byte]);
}

TEST_F(BitmapPageAllocator, KeepsDisjointUsableRegionsSeparateFromReservedGaps)
{
    initialize({{physicalBase, 16 * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE},
                {physicalBase + 16 * G_PAGE_SIZE, 16 * G_PAGE_SIZE, 1},
                {physicalBase + 32 * G_PAGE_SIZE, 12 * G_PAGE_SIZE, LIMINE_MEMMAP_USABLE}});
    auto expected = expectedPages(16);
    for(unsigned page = 33; page < 44; ++page) expected.insert(physicalBase + page * G_PAGE_SIZE);
    expectUniquePages(drain(), expected);
}

TEST_F(BitmapPageAllocator, AccountsForExactCapacityAcross64BitWordBoundaries)
{
    for(uint32_t frames : {0U, 1U, 63U, 64U, 65U, 127U, 128U, 129U})
    {
        SCOPED_TRACE(frames);
        initializePages(frames + 2);
        ASSERT_EQ(frames, allocator.freePageCount);
        expectUniquePages(drain(), expectedPages(frames + 2));
    }
}

TEST_F(BitmapPageAllocator, AllocatesBothTheHighestWordBitAndTheNextWord)
{
    initializePages(68);
    for(unsigned bit = 0; bit < 66; ++bit)
        EXPECT_EQ(physicalBase + (bit + 1) * G_PAGE_SIZE, bitmapPageAllocatorTryAllocate(&allocator, 0));
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 0));
}

TEST_F(BitmapPageAllocator, StartsAnotherBitmapOnlyWhenItCanDescribeAFrame)
{
    const uint32_t capacity = G_BITMAP_TOTAL_BITS;
    initializePages(capacity + 2);
    EXPECT_EQ(capacity, allocator.freePageCount);
    EXPECT_EQ(nullptr, allocator.indexPage->entries[1]);
    initializePages(capacity + 3);
    EXPECT_EQ(capacity, allocator.freePageCount);
    EXPECT_EQ(nullptr, allocator.indexPage->entries[1]);
    initializePages(capacity + 4);
    EXPECT_EQ(capacity + 1, allocator.freePageCount);
    ASSERT_NE(nullptr, allocator.indexPage->entries[1]);
    const auto bitmap = allocator.indexPage->entries[1];
    EXPECT_EQ(G_PAGE_SIZE, bitmap->end - bitmap->base);
}

TEST_F(BitmapPageAllocator, StopsAtReserveAndReusesBudgetAfterFree)
{
    initializePages(12);
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 10));
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 11));
    auto page = bitmapPageAllocatorTryAllocate(&allocator, 9);
    ASSERT_NE(0U, page);
    EXPECT_EQ(9U, allocator.freePageCount);
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 9));
    bitmapPageAllocatorMarkFree(&allocator, page);
    EXPECT_EQ(10U, allocator.freePageCount);
    EXPECT_EQ(page, bitmapPageAllocatorTryAllocate(&allocator, 9));
    EXPECT_EQ(9U, allocator.freePageCount);
}

TEST_F(BitmapPageAllocator, ReusesFastBufferPagesInLifoOrderWithoutDoubleCharging)
{
    initializePages(12);
    std::vector<uintptr_t> pages;
    for(unsigned i = 0; i < 3; ++i) pages.push_back(bitmapPageAllocatorAllocate(&allocator));
    for(auto page : pages) bitmapPageAllocatorMarkFree(&allocator, page);
    ASSERT_EQ(3, allocator.fastBuffer.size);
    EXPECT_EQ(10U, allocator.freePageCount);
    for(auto it = pages.rbegin(); it != pages.rend(); ++it)
        EXPECT_EQ(*it, bitmapPageAllocatorTryAllocate(&allocator, 0));
    EXPECT_EQ(0, allocator.fastBuffer.size);
    EXPECT_EQ(7U, allocator.freePageCount);
}

TEST_F(BitmapPageAllocator, SpillsFullFastBufferBackToBitmapWithoutLosingFrames)
{
    initializePages(162);
    auto pages = drain();
    ASSERT_EQ(160U, pages.size());
    for(auto page : pages) bitmapPageAllocatorMarkFree(&allocator, page);
    EXPECT_EQ(G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE, allocator.fastBuffer.size);
    EXPECT_EQ(160U, allocator.freePageCount);
    expectUniquePages(drain(), expectedPages(162));
    EXPECT_EQ(0, allocator.fastBuffer.size);
}

TEST_F(BitmapPageAllocator, RefundsReservedBudgetWhenNoBackingFrameCanBeClaimed)
{
    initializePages(2); // Valid metadata but no managed frames.
    allocator.freePageCount = 1; // Inject a failed claim after a budget reservation.
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 0));
    EXPECT_EQ(1U, allocator.freePageCount);
}

TEST_F(BitmapPageAllocator, ReturnsZeroOnOrdinaryExhaustionAndPanicsForCriticalAllocation)
{
    initializePages(2);
    EXPECT_EQ(0U, bitmapPageAllocatorTryAllocate(&allocator, 0));
    EXPECT_EQ(0U, allocator.freePageCount);
    EXPECT_DEATH(bitmapPageAllocatorAllocate(&allocator), "");
}

TEST_F(BitmapPageAllocator, PanicsWhenNoRegionCanProvideBootstrapMetadata)
{
    EXPECT_DEATH(initialize({}), "");
    EXPECT_DEATH(initializePages(1), "");
}

TEST_F(BitmapPageAllocator, RejectsUnalignedFrees)
{
    initializePages(12);
    auto page = bitmapPageAllocatorAllocate(&allocator);
    EXPECT_DEATH(bitmapPageAllocatorMarkFree(&allocator, page + 1), "");
    EXPECT_EQ(9U, allocator.freePageCount);
}

TEST_F(BitmapPageAllocator, RejectsUnmanagedAndMetadataPagesBeforeTheyReachFastBuffer)
{
    initializePages(12);
    for(auto invalid : std::initializer_list<uintptr_t>{0, physicalBase, physicalBase + 11 * G_PAGE_SIZE,
                        physicalBase + 12 * G_PAGE_SIZE, physicalBase - G_PAGE_SIZE})
    {
        SCOPED_TRACE(invalid);
        bitmapPageAllocatorMarkFree(&allocator, invalid);
        EXPECT_EQ(10U, allocator.freePageCount);
        EXPECT_EQ(0, allocator.fastBuffer.size);
    }
    expectUniquePages(drain(), expectedPages(12));
}

TEST_F(BitmapPageAllocator, RejectsFreeOfAFrameAlreadyAvailableInBitmap)
{
    initializePages(12);
    bitmapPageAllocatorMarkFree(&allocator, physicalBase + G_PAGE_SIZE);
    EXPECT_EQ(10U, allocator.freePageCount);
    EXPECT_EQ(0, allocator.fastBuffer.size);
    expectUniquePages(drain(), expectedPages(12));
}

TEST_F(BitmapPageAllocator, RejectsDuplicateFastBufferFreesWithoutDuplicatingAllocations)
{
    initializePages(12);
    auto page = bitmapPageAllocatorAllocate(&allocator);
    bitmapPageAllocatorMarkFree(&allocator, page);
    bitmapPageAllocatorMarkFree(&allocator, page);
    EXPECT_EQ(10U, allocator.freePageCount);
    EXPECT_EQ(1, allocator.fastBuffer.size);
    expectUniquePages(drain(), expectedPages(12));
}

TEST_F(BitmapPageAllocator, RejectsDuplicateBitmapFreesWhenFastBufferIsFull)
{
    initializePages(162);
    auto pages = drain();
    ASSERT_EQ(160U, pages.size());
    for(unsigned i = 0; i <= G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE; ++i)
        bitmapPageAllocatorMarkFree(&allocator, pages[i]);
    bitmapPageAllocatorMarkFree(&allocator, pages[G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE]);
    EXPECT_EQ(G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE + 1U, allocator.freePageCount);
    std::set<uintptr_t> expected(pages.begin(), pages.begin() + G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE + 1);
    expectUniquePages(drain(), expected);
}

TEST_F(BitmapPageAllocator, SerializesSimultaneousFreesOfTheSameFrame)
{
    initializePages(12);
    auto page = bitmapPageAllocatorAllocate(&allocator);
    std::vector<std::thread> threads;
    for(unsigned i = 0; i < 8; ++i)
        threads.emplace_back([&] { bitmapPageAllocatorMarkFree(&allocator, page); });
    for(auto& thread : threads) thread.join();
    EXPECT_EQ(10U, allocator.freePageCount);
    EXPECT_EQ(1, allocator.fastBuffer.size);
    expectUniquePages(drain(), expectedPages(12));
}

TEST_F(BitmapPageAllocator, PreservesOwnershipAndContentsAcrossConcurrentAllocateFreeCycles)
{
    initializePages(66);
    std::mutex ownersLock;
    std::set<uintptr_t> owned;
    std::vector<std::thread> threads;
    for(unsigned id = 0; id < 8; ++id)
        threads.emplace_back([&, id] {
            for(unsigned round = 0; round < 250; ++round)
            {
                auto page = bitmapPageAllocatorTryAllocate(&allocator, 0);
                EXPECT_NE(0U, page);
                if(!page) continue;
                {
                    std::lock_guard<std::mutex> guard(ownersLock);
                    EXPECT_TRUE(owned.insert(page).second);
                }
                auto content = reinterpret_cast<uint64_t*>(G_MEM_PHYS_TO_VIRT(page));
                const uint64_t tag = (uint64_t(id) << 32) | round;
                *content = tag;
                std::this_thread::yield();
                EXPECT_EQ(tag, *content);
                {
                    std::lock_guard<std::mutex> guard(ownersLock);
                    EXPECT_EQ(1U, owned.erase(page));
                }
                bitmapPageAllocatorMarkFree(&allocator, page);
            }
        });
    for(auto& thread : threads) thread.join();
    EXPECT_TRUE(owned.empty());
    EXPECT_EQ(64U, allocator.freePageCount);
    expectUniquePages(drain(), expectedPages(66));
}

TEST_F(BitmapPageAllocator, KeepsInPageMutexAnd64BitBitmapEntriesAligned)
{
    EXPECT_EQ(0U, offsetof(g_bitmap_header, lock) % alignof(g_mutex));
    EXPECT_EQ(0U, offsetof(g_bitmap_header, entries) % alignof(uint64_t));
    EXPECT_LE(offsetof(g_bitmap_header, entries) + G_BITMAP_MAX_ENTRIES * sizeof(uint64_t), G_PAGE_SIZE);
    EXPECT_LE(offsetof(g_bitmap_index_page_header, entries) + G_BITMAP_INDEX_MAX_ENTRIES * sizeof(g_bitmap_header*), G_PAGE_SIZE);
}
}
