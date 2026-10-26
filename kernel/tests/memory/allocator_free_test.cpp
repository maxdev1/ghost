#include "support/fixture.hpp"

namespace
{
#include "support/memory/heap_allocator.hpp"

TEST_F(HeapAllocator, RejectsNullAndUnmanagedPointersWithoutChangingLiveAllocation)
{
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    std::memset(memory, 0xa6, 32);
    for(void* invalid : std::array<void*, 3>{nullptr, storage, arena + activeSize})
        EXPECT_EQ(0U, memoryAllocatorFree(&allocator, invalid));
    expectPattern(memory, 32, 0xa6);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, memory));
}

TEST_F(HeapAllocator, RejectsInteriorBucketPointersWithoutReleasingLiveSlot)
{
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    std::memset(memory, 0x83, 32);
    for(unsigned offset : {1U, 16U, 31U})
        EXPECT_EQ(0U, memoryAllocatorFree(&allocator, static_cast<uint8_t*>(memory) + offset));
    auto other = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, other);
    EXPECT_NE(memory, other);
    expectPattern(memory, 32, 0x83);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, memory));
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, other));
}

TEST_F(HeapAllocator, RejectsBucketMetadataPointersBeforeCalculatingBitmapIndex)
{
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    auto header = reinterpret_cast<uint8_t*>(allocator.sections);
    for(auto invalid : {header + 1, header + sizeof(g_allocator_section_header),
                       header + sizeof(g_allocator_section_bucket), static_cast<uint8_t*>(memory) - 1})
        EXPECT_EQ(0U, memoryAllocatorFree(&allocator, invalid));
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, memory));
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, RejectsBucketTailBeyondTheLastSlot)
{
    initialize(9008); // Fits a 32-byte bucket with an unsplittable trailing remainder.
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    std::memset(memory, 0x91, 32);
    auto tail = static_cast<uint8_t*>(memory) + 256 * 32;
    ASSERT_LT(reinterpret_cast<uintptr_t>(tail), reinterpret_cast<uintptr_t>(arena + activeSize));
    EXPECT_EQ(0U, memoryAllocatorFree(&allocator, tail));
    expectPattern(memory, 32, 0x91);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, memory));
}

TEST_F(HeapAllocator, RejectsBucketSlotThatWasNeverAllocated)
{
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    auto unused = static_cast<uint8_t*>(memory) + 32;
    EXPECT_EQ(0U, memoryAllocatorFree(&allocator, unused));
    EXPECT_EQ(unused, memoryAllocatorAllocate(&allocator, 32));
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, memory));
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, unused));
}

TEST_F(HeapAllocator, RejectsDuplicateBucketFreeWhileOtherSlotsRemainLive)
{
    auto first = memoryAllocatorAllocate(&allocator, 32);
    auto second = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    std::memset(second, 0xb7, 32);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, first));
    EXPECT_EQ(0U, memoryAllocatorFree(&allocator, first));
    expectPattern(second, 32, 0xb7);
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, second));
}

TEST_F(HeapAllocator, RejectsInteriorChunkPointersAndChunkMetadata)
{
    auto memory = memoryAllocatorAllocate(&allocator, 4096);
    ASSERT_NE(nullptr, memory);
    auto header = reinterpret_cast<uint8_t*>(allocator.sections);
    std::memset(memory, 0xe3, 4096);
    for(auto invalid : {header + 1, header + sizeof(g_allocator_section_header) - 1,
                       static_cast<uint8_t*>(memory) + 1, static_cast<uint8_t*>(memory) + 16})
        EXPECT_EQ(0U, memoryAllocatorFree(&allocator, invalid));
    expectPattern(memory, 4096, 0xe3);
    EXPECT_EQ(4096U, memoryAllocatorFree(&allocator, memory));
}

TEST_F(HeapAllocator, RejectsRepeatedChunkFreeAfterCoalescing)
{
    auto memory = memoryAllocatorAllocate(&allocator, 4096);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(4096U, memoryAllocatorFree(&allocator, memory));
    EXPECT_EQ(0U, memoryAllocatorFree(&allocator, memory));
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, RejectsPointerOwnedByAnotherAllocator)
{
    alignas(16) uint8_t otherArena[16384];
    g_allocator other{};
    memoryAllocatorInitialize(&other, G_ALLOCATOR_TYPE_LOWERMEM,
        reinterpret_cast<uintptr_t>(otherArena), reinterpret_cast<uintptr_t>(otherArena + sizeof(otherArena)));
    auto memory = memoryAllocatorAllocate(&other, 32);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(0U, memoryAllocatorFree(&allocator, memory));
    EXPECT_EQ(32U, memoryAllocatorFree(&other, memory));
    expectFullyReclaimed();
}
}
