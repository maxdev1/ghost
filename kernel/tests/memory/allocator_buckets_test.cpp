#include "support/fixture.hpp"

namespace
{
#include "support/memory/heap_allocator.hpp"

TEST_F(HeapAllocator, ZeroSizeReturnsOneMinimumAlignedSlot)
{
    g_size capacity = 0;
    auto memory = memoryAllocatorAllocate(&allocator, 0, &capacity);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(16U, capacity);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(memory) % 16);
    EXPECT_EQ(capacity, memoryAllocatorFree(&allocator, memory));
}

TEST_F(HeapAllocator, ReportsAlignedCapacityAtEverySizeClassBoundary)
{
    const std::pair<size_t, size_t> cases[] = {
        {1, 16}, {15, 16}, {16, 16}, {17, 32}, {31, 32}, {32, 32},
        {33, 64}, {63, 64}, {64, 64}, {65, 128}, {127, 128}, {128, 128},
        {129, 256}, {255, 256}, {256, 256}, {257, 512}, {511, 512}, {512, 512},
        {513, 1024}, {1023, 1024}, {1024, 1024}, {1025, 2048}, {4097, 8192}
    };
    for(auto test : cases)
    {
        SCOPED_TRACE(test.first);
        g_size capacity = 0;
        auto memory = memoryAllocatorAllocate(&allocator, test.first, &capacity);
        ASSERT_NE(nullptr, memory);
        EXPECT_EQ(test.second, capacity);
        EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(memory) % 16);
        std::memset(memory, 0x73, capacity);
        EXPECT_EQ(capacity, memoryAllocatorFree(&allocator, memory));
    }
}

TEST_F(HeapAllocator, AddsAnotherBucketWhenAllSlotsAreLive)
{
    std::unordered_set<void*> owned;
    std::vector<void*> slots;
    // A 1024-byte bucket has 16 slots; the seventeenth needs another bucket.
    for(unsigned i = 0; i < 17; ++i)
    {
        auto memory = memoryAllocatorAllocate(&allocator, 1024);
        ASSERT_NE(nullptr, memory);
        ASSERT_TRUE(owned.insert(memory).second);
        std::memset(memory, i, 1024);
        slots.push_back(memory);
    }
    size_t buckets = 0;
    for(auto section = allocator.sections; section; section = section->next)
        if(section->type == G_ALLOCATOR_SECTION_TYPE_BUCKET) ++buckets;
    EXPECT_EQ(2U, buckets);
    for(unsigned i = 0; i < slots.size(); ++i)
    {
        expectPattern(slots[i], 1024, i);
        EXPECT_EQ(1024U, memoryAllocatorFree(&allocator, slots[i]));
    }
}

TEST_F(HeapAllocator, ReusesFreedSlotsAcrossBitmapByteBoundaries)
{
    std::vector<void*> slots;
    for(unsigned i = 0; i < 128; ++i)
    {
        auto memory = memoryAllocatorAllocate(&allocator, 64);
        ASSERT_NE(nullptr, memory);
        slots.push_back(memory);
    }
    std::unordered_set<void*> holes;
    for(unsigned index : {0U, 7U, 8U, 127U})
    {
        holes.insert(slots[index]);
        EXPECT_EQ(64U, memoryAllocatorFree(&allocator, slots[index]));
    }
    for(unsigned i = 0; i < 4; ++i)
    {
        auto memory = memoryAllocatorAllocate(&allocator, 64);
        EXPECT_EQ(1U, holes.erase(memory));
    }
    EXPECT_TRUE(holes.empty());
    for(auto memory : slots) EXPECT_EQ(64U, memoryAllocatorFree(&allocator, memory));
}

TEST_F(HeapAllocator, KeepsPartlyUsedBucketAndPreservesItsOtherSlots)
{
    auto first = memoryAllocatorAllocate(&allocator, 32);
    auto second = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    std::memset(second, 0x6b, 32);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, first));
    EXPECT_EQ(G_ALLOCATOR_SECTION_TYPE_BUCKET, allocator.sections->type);
    auto reused = memoryAllocatorAllocate(&allocator, 32);
    EXPECT_EQ(first, reused);
    expectPattern(second, 32, 0x6b);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, reused));
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, second));
}

TEST_F(HeapAllocator, LastBucketFreeMakesSpaceAvailableForLargerChunk)
{
    initialize(16384);
    auto small = memoryAllocatorAllocate(&allocator, 16);
    ASSERT_NE(nullptr, small);
    EXPECT_EQ(16U, memoryAllocatorFree(&allocator, small));
    expectFullyReclaimed();
    auto large = memoryAllocatorAllocate(&allocator, 8192);
    ASSERT_NE(nullptr, large);
    EXPECT_EQ(8192U, memoryAllocatorFree(&allocator, large));
}

TEST_F(HeapAllocator, ReclaimsBucketsAcrossChangingSizeClasses)
{
    initialize(32768);
    for(size_t size : {16U, 32U, 64U, 128U, 256U, 512U, 1024U})
    {
        SCOPED_TRACE(size);
        auto memory = memoryAllocatorAllocate(&allocator, size);
        ASSERT_NE(nullptr, memory);
        EXPECT_EQ(size, memoryAllocatorFree(&allocator, memory));
        expectFullyReclaimed();
    }
    auto large = memoryAllocatorAllocate(&allocator, 16384);
    ASSERT_NE(nullptr, large);
    EXPECT_EQ(16384U, memoryAllocatorFree(&allocator, large));
}

TEST_F(HeapAllocator, FailedBucketCreationLeavesHeapUsableForChunks)
{
    initialize(4096);
    g_size capacity = 99;
    EXPECT_EQ(nullptr, memoryAllocatorAllocate(&allocator, 16, &capacity));
    EXPECT_EQ(0U, capacity);
    expectFullyReclaimed();
    auto memory = memoryAllocatorAllocate(&allocator, 2048);
    ASSERT_NE(nullptr, memory);
    EXPECT_GE(memoryAllocatorFree(&allocator, memory), 2048U);
}
}
