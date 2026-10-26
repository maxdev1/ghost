#include "support/fixture.hpp"

namespace
{
#include "support/memory/heap_allocator.hpp"

TEST_F(HeapAllocator, SplitsChunksWithoutOverlappingTheirPayloads)
{
    initialize(16384);
    g_size firstCapacity, secondCapacity;
    auto first = memoryAllocatorAllocate(&allocator, 2048, &firstCapacity);
    auto second = memoryAllocatorAllocate(&allocator, 4096, &secondCapacity);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(2048U, firstCapacity);
    EXPECT_EQ(4096U, secondCapacity);
    EXPECT_LE(reinterpret_cast<uintptr_t>(first) + firstCapacity, reinterpret_cast<uintptr_t>(second));
    std::memset(first, 0x1a, firstCapacity);
    std::memset(second, 0xb2, secondCapacity);
    expectPattern(first, firstCapacity, 0x1a);
    EXPECT_EQ(firstCapacity, memoryAllocatorFree(&allocator, first));
    expectPattern(second, secondCapacity, 0xb2);
    EXPECT_EQ(secondCapacity, memoryAllocatorFree(&allocator, second));
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, CoalescesChunksInEveryFreeOrder)
{
    std::array<unsigned, 3> order{0, 1, 2};
    do
    {
        initialize(32768);
        std::array<void*, 3> memory;
        for(auto& entry : memory)
        {
            entry = memoryAllocatorAllocate(&allocator, 4096);
            ASSERT_NE(nullptr, entry);
        }
        for(auto index : order)
        {
            EXPECT_EQ(4096U, memoryAllocatorFree(&allocator, memory[index]));
            expectLayout();
        }
        expectFullyReclaimed();
        auto large = memoryAllocatorAllocate(&allocator, 16384);
        ASSERT_NE(nullptr, large);
        EXPECT_EQ(memory[0], large);
        EXPECT_EQ(16384U, memoryAllocatorFree(&allocator, large));
    } while(std::next_permutation(order.begin(), order.end()));
}

TEST_F(HeapAllocator, DoesNotMergeAcrossLiveChunkDuringFragmentedExhaustion)
{
    initialize(14336); // Enough total free space for 8192, but neither separated gap fits.
    auto left = memoryAllocatorAllocate(&allocator, 2048);
    auto middle = memoryAllocatorAllocate(&allocator, 4096);
    auto right = memoryAllocatorAllocate(&allocator, 2048);
    ASSERT_NE(nullptr, left);
    ASSERT_NE(nullptr, middle);
    ASSERT_NE(nullptr, right);
    std::memset(middle, 0xc5, 4096);
    memoryAllocatorFree(&allocator, left);
    memoryAllocatorFree(&allocator, right);
    g_size capacity = 99;
    EXPECT_EQ(nullptr, memoryAllocatorAllocate(&allocator, 8192, &capacity));
    EXPECT_EQ(0U, capacity);
    expectPattern(middle, 4096, 0xc5);
    EXPECT_EQ(4096U, memoryAllocatorFree(&allocator, middle));
    auto recovered = memoryAllocatorAllocate(&allocator, 8192);
    ASSERT_NE(nullptr, recovered);
    EXPECT_EQ(8192U, memoryAllocatorFree(&allocator, recovered));
}

TEST_F(HeapAllocator, ExactFitConsumesOneSectionAndRecoversAfterExhaustion)
{
    initialize(sizeof(g_allocator_section_header) + 4096);
    g_size capacity = 0;
    auto memory = memoryAllocatorAllocate(&allocator, 4096, &capacity);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(4096U, capacity);
    EXPECT_EQ(nullptr, allocator.sections->next);
    EXPECT_EQ(nullptr, memoryAllocatorAllocate(&allocator, 4096));
    EXPECT_EQ(capacity, memoryAllocatorFree(&allocator, memory));
    EXPECT_EQ(memory, memoryAllocatorAllocate(&allocator, 4096));
}

TEST_F(HeapAllocator, ReportsCapacityOnBothSidesOfMinimumSplitRemainder)
{
    const size_t threshold = sizeof(g_allocator_section_header) + 1024;
    for(auto remainder : {threshold - 16, threshold, threshold + 16})
    {
        SCOPED_TRACE(remainder);
        initialize(sizeof(g_allocator_section_header) + 2048 + remainder);
        g_size capacity = 0;
        auto memory = memoryAllocatorAllocate(&allocator, 2048, &capacity);
        ASSERT_NE(nullptr, memory);
        EXPECT_EQ(remainder > threshold ? 2048U : 2048U + remainder, capacity);
        EXPECT_EQ(capacity, memoryAllocatorFree(&allocator, memory));
        expectFullyReclaimed();
    }
}

TEST_F(HeapAllocator, ExpansionCoalescesWithExistingFreeTail)
{
    initialize(4096);
    memoryAllocatorExpand(&allocator, 12288);
    activeSize += 12288;
    expectFullyReclaimed();
    auto memory = memoryAllocatorAllocate(&allocator, 8192);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(8192U, memoryAllocatorFree(&allocator, memory));
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, ExpansionAfterAllocatedTailPreservesLiveContents)
{
    initialize(sizeof(g_allocator_section_header) + 4096);
    auto first = memoryAllocatorAllocate(&allocator, 4096);
    ASSERT_NE(nullptr, first);
    std::memset(first, 0x8c, 4096);
    memoryAllocatorExpand(&allocator, 8192);
    activeSize += 8192;
    auto second = memoryAllocatorAllocate(&allocator, 4096);
    ASSERT_NE(nullptr, second);
    EXPECT_NE(first, second);
    std::memset(second, 0x39, 4096);
    expectPattern(first, 4096, 0x8c);
    memoryAllocatorFree(&allocator, first);
    expectPattern(second, 4096, 0x39);
    memoryAllocatorFree(&allocator, second);
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, EmptyBucketCoalescesWithFreeChunksOnBothSides)
{
    auto left = memoryAllocatorAllocate(&allocator, 4096);
    auto middle = memoryAllocatorAllocate(&allocator, 64);
    auto right = memoryAllocatorAllocate(&allocator, 4096);
    ASSERT_NE(nullptr, left);
    ASSERT_NE(nullptr, middle);
    ASSERT_NE(nullptr, right);
    memoryAllocatorFree(&allocator, left);
    memoryAllocatorFree(&allocator, right);
    EXPECT_EQ(64U, memoryAllocatorFree(&allocator, middle));
    expectFullyReclaimed();
}
}
