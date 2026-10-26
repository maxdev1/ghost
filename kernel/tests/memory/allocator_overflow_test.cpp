#include "support/fixture.hpp"

namespace
{
uint64_t mathCeilToPowerOf2(uint64_t);
#include "production/allocator_hpp.inc"
#include "production/math_cpp.inc"
#include "production/allocator_cpp.inc"

TEST(HeapAllocator, RejectsOversizedRequestsWithoutDamagingAllocator)
{
    alignas(16) uint8_t arena[16384]; g_allocator allocator;
    memoryAllocatorInitialize(&allocator, G_ALLOCATOR_TYPE_HEAP, (uintptr_t)arena, (uintptr_t)(arena + sizeof(arena)));
    for(uint64_t size : std::initializer_list<uint64_t>{0x80000001ULL, 0x100000000ULL, 0x8000000000000001ULL, UINT64_MAX}) {
        g_size capacity = 99;
        EXPECT_TRUE(!memoryAllocatorAllocate(&allocator, size, &capacity) && !capacity);
    }
    auto valid = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, valid);
    EXPECT_GE(memoryAllocatorFree(&allocator, valid), 32U);
}
}
