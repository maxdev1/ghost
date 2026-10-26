#include "support/fixture.hpp"

namespace
{
#include "support/synchronization.hpp"

uint64_t mathCeilToPowerOf2(uint64_t);

#include "production/allocator_hpp.inc"
#include "production/math_cpp.inc"
#include "production/allocator_cpp.inc"
#include "production/lower_heap_cpp.inc"
#include "production/heap_host_cpp.inc"

class HeapAccounting : public testing::Test
{
protected:
    alignas(16) inline static uint8_t mainArena[2 * 1024 * 1024], lowerArena[2 * 1024 * 1024];

    void SetUp() override
    {
        heapAmountInUse = lowerHeapAmountInUse = 0;
        heapInitialized = true;
        mutexInitializeGlobal(&heapLock);
        memoryAllocatorInitialize(&heapAllocator, G_ALLOCATOR_TYPE_HEAP,
            (uintptr_t)mainArena, (uintptr_t)(mainArena + sizeof(mainArena)));
        lowerHeapInitialize((uintptr_t)lowerArena, (uintptr_t)(lowerArena + sizeof(lowerArena)));
    }

    void TearDown() override
    {
        EXPECT_EQ(0U, heapGetUsedAmount());
        EXPECT_EQ(0U, lowerHeapGetUsedAmount());
        EXPECT_EQ(0, heapLock.depth);
        EXPECT_EQ(0, heapAllocator.lock.depth);
        EXPECT_EQ(0, lowerHeapMutex.depth);
        EXPECT_EQ(0, lowerHeapAllocator.lock.depth);
    }
};

TEST_F(HeapAccounting, BalancesMainAndLowHeapUsageAcrossRepeatedAllocations)
{
    const uint32_t sizes[] = {0, 1, 17, 28, 513, 1000, 1025, 4097, 10000};
    for(int round = 0; round < 100; ++round) {
        std::vector<void*> mainBlocks, lowerBlocks;
        for(auto size : sizes) {
            mainBlocks.push_back(heapAllocate(size));
            lowerBlocks.push_back(lowerHeapAllocate(size));
        }
        auto total = heapGetUsedAmount(); EXPECT_TRUE(total >= 20000);
        for(auto memory : mainBlocks) { heapFree(memory); EXPECT_TRUE(heapGetUsedAmount() < total); }
        for(auto memory : lowerBlocks) lowerHeapFree(memory);
        EXPECT_TRUE(heapGetUsedAmount() == 0 && lowerHeapGetUsedAmount() == 0);
    }
}

TEST_F(HeapAccounting, InvalidAndDuplicateMainHeapFreesDoNotUnderflowUsage)
{
    auto first = heapAllocate(32);
    auto second = heapAllocate(32);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    heapFree(static_cast<uint8_t*>(first) + 1);
    heapFree(nullptr);
    heapFree(lowerArena);
    EXPECT_EQ(64U, heapGetUsedAmount());
    heapFree(first);
    EXPECT_EQ(32U, heapGetUsedAmount());
    heapFree(first);
    EXPECT_EQ(32U, heapGetUsedAmount());
    heapFree(second);
    EXPECT_EQ(0U, heapGetUsedAmount());
}

TEST_F(HeapAccounting, InvalidAndDuplicateLowerHeapFreesDoNotUnderflowUsage)
{
    auto first = lowerHeapAllocate(32);
    auto second = lowerHeapAllocate(32);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    lowerHeapFree(static_cast<uint8_t*>(first) + 16);
    lowerHeapFree(nullptr);
    lowerHeapFree(mainArena);
    EXPECT_EQ(64U, lowerHeapGetUsedAmount());
    lowerHeapFree(first);
    EXPECT_EQ(32U, lowerHeapGetUsedAmount());
    lowerHeapFree(first);
    EXPECT_EQ(32U, lowerHeapGetUsedAmount());
    lowerHeapFree(second);
    EXPECT_EQ(0U, lowerHeapGetUsedAmount());
}

TEST_F(HeapAccounting, ReclaimedBucketSatisfiesLargerMainHeapRequestWithoutExpansion)
{
    memoryAllocatorInitialize(&heapAllocator, G_ALLOCATOR_TYPE_HEAP,
        (uintptr_t)mainArena, (uintptr_t)(mainArena + 16384));
    auto small = heapAllocate(16);
    ASSERT_NE(nullptr, small);
    EXPECT_EQ(16U, heapGetUsedAmount());
    heapFree(small);
    EXPECT_EQ(0U, heapGetUsedAmount());
    auto large = heapAllocate(8192); // Host fixture cannot expand: reclamation must suffice.
    ASSERT_NE(nullptr, large);
    EXPECT_EQ(8192U, heapGetUsedAmount());
    heapFree(large);
}
}
