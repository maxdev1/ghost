#include "support/fixture.hpp"

namespace
{
#include "support/memory/heap_allocator.hpp"

TEST_F(HeapAllocator, SeededMixedLifetimeTracePreservesContentsAndReclaimsEverySection)
{
    struct Allocation { void* memory; g_size capacity; uint8_t pattern; };
    std::vector<Allocation> live;
    uint32_t seed = 0x42a57b19;
    auto next = [&] { seed = seed * 1664525U + 1013904223U; return seed; };
    const size_t sizes[] = {1, 17, 63, 65, 129, 257, 513, 1025, 2049, 4097, 8193};
    unsigned successes = 0;
    for(unsigned step = 0; step < 4000; ++step)
    {
        SCOPED_TRACE(step);
        if(!live.empty() && (next() % 100 < 47 || live.size() >= 48))
        {
            const size_t index = next() % live.size();
            auto item = live[index];
            expectPattern(item.memory, item.capacity, item.pattern);
            EXPECT_EQ(item.capacity, memoryAllocatorFree(&allocator, item.memory));
            live.erase(live.begin() + index);
        }
        else
        {
            const auto requested = sizes[next() % std::size(sizes)];
            g_size capacity = 99;
            auto memory = memoryAllocatorAllocate(&allocator, requested, &capacity);
            if(memory)
            {
                ++successes;
                ASSERT_GE(capacity, requested);
                const auto address = reinterpret_cast<uintptr_t>(memory);
                ASSERT_GE(address, reinterpret_cast<uintptr_t>(arena));
                ASSERT_LE(address + capacity, reinterpret_cast<uintptr_t>(arena + activeSize));
                EXPECT_EQ(0U, address % 16);
                for(auto item : live)
                {
                    const auto other = reinterpret_cast<uintptr_t>(item.memory);
                    ASSERT_TRUE(address + capacity <= other || other + item.capacity <= address);
                }
                const uint8_t pattern = 1 + next() % 255;
                std::memset(memory, pattern, capacity);
                live.push_back({memory, capacity, pattern});
            }
            else
                EXPECT_EQ(0U, capacity);
        }
        if(step % 32 == 0)
        {
            for(auto item : live) expectPattern(item.memory, item.capacity, item.pattern);
            expectLayout();
        }
    }
    EXPECT_GT(successes, 1000U);
    for(auto item : live)
    {
        expectPattern(item.memory, item.capacity, item.pattern);
        EXPECT_EQ(item.capacity, memoryAllocatorFree(&allocator, item.memory));
    }
    expectFullyReclaimed();
}
}
