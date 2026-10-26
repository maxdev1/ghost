#include "support/fixture.hpp"

namespace
{
uint64_t mathCeilToPowerOf2(uint64_t);

#include "production/allocator_hpp.inc"
#include "production/math_cpp.inc"
#include "production/allocator_cpp.inc"

TEST(HeapAllocator, AlignsMixedAllocationsPreservesContentsAndCoalescesFreedChunks) {
    alignas(16) static unsigned char arena[4 * 1024 * 1024];
    g_allocator allocator;
    memoryAllocatorInitialize(&allocator, G_ALLOCATOR_TYPE_HEAP,
        reinterpret_cast<uintptr_t>(arena), reinterpret_cast<uintptr_t>(arena + sizeof(arena)));
    std::vector<std::pair<void*, size_t>> allocations;
    const size_t sizes[] = {1, 8, 15, 16, 17, 28, 48, 64, 100, 256, 257, 512, 1024, 1025, 4096, 10000};
    // Mix buckets and chunks, then reclaim and reuse them repeatedly.
    for(int round = 0; round < 4; ++round) {
        for(size_t size : sizes) {
            for(int i = 0; i < 35; ++i) {
                void* p = memoryAllocatorAllocate(&allocator, size);
                ASSERT_NE(nullptr, p);
                EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(p) % 16);
                std::memset(p, 0xa5, size);
                allocations.emplace_back(p, size);
            }
        }
        for(auto allocation : allocations) {
            auto p = static_cast<unsigned char*>(allocation.first);
            for(size_t i = 0; i < allocation.second; ++i) EXPECT_EQ(0xa5, p[i]);
            EXPECT_TRUE(memoryAllocatorFree(&allocator, p) >= allocation.second);
        }
        allocations.clear();
    }
    // Freed chunks must merge so a large allocation can reuse them.
    auto p = memoryAllocatorAllocate(&allocator, sizeof(arena) / 2);
    ASSERT_NE(nullptr, p);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(p) % 16);
    EXPECT_GE(memoryAllocatorFree(&allocator, p), sizeof(arena) / 2);
}
}
