#include "support/fixture.hpp"

namespace
{
uint64_t mathCeilToPowerOf2(uint64_t);
#include "production/allocator_hpp.inc"
#include "production/math_cpp.inc"
#include "production/allocator_cpp.inc"

TEST(HeapAllocator, ReportsUnsplittableCapacityAndClearsCapacityOnFailure)
{
    // Capacity includes an unsplittable remainder, rather than just size rounding.
    alignas(16) uint8_t arena[2208];
    g_allocator allocator;
    memoryAllocatorInitialize(&allocator, G_ALLOCATOR_TYPE_HEAP, (uintptr_t)arena, (uintptr_t)(arena + sizeof(arena)));
    g_size allocated = 0;
    auto memory = memoryAllocatorAllocate(&allocator, 1025, &allocated);
    EXPECT_TRUE(memory && allocated > 2048);
    EXPECT_EQ(allocated, memoryAllocatorFree(&allocator, memory));
    allocated = 999;
    EXPECT_TRUE(!memoryAllocatorAllocate(&allocator, 8192, &allocated) && allocated == 0);
}
}
