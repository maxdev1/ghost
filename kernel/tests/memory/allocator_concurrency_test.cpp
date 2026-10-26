#include "support/fixture.hpp"

namespace
{
#include "support/memory/heap_allocator.hpp"

TEST_F(HeapAllocator, ConcurrentAllocationsPreserveExclusiveOwnershipAndContents)
{
    struct Owner { uintptr_t start; g_size capacity; };
    std::vector<Owner> owners;
    std::mutex ownersLock;
    std::vector<std::thread> workers;
    const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
    for(unsigned id = 0; id < 8; ++id)
        workers.emplace_back([&, id] {
            for(unsigned round = 0; round < 300; ++round)
            {
                const size_t requested = sizes[(round + id) % std::size(sizes)];
                g_size capacity = 0;
                auto memory = memoryAllocatorAllocate(&allocator, requested, &capacity);
                EXPECT_NE(nullptr, memory);
                if(!memory) continue;
                const auto start = reinterpret_cast<uintptr_t>(memory);
                bool exclusive = true;
                {
                    std::lock_guard<std::mutex> guard(ownersLock);
                    for(auto owner : owners)
                        if(start < owner.start + owner.capacity && owner.start < start + capacity)
                            exclusive = false;
                    EXPECT_TRUE(exclusive) << "allocator returned overlapping live payloads";
                    if(exclusive) owners.push_back({start, capacity});
                }
                if(!exclusive) continue; // Avoid writing through a pointer another worker owns.
                EXPECT_GE(capacity, requested);
                const uint8_t pattern = 1 + id * 23 + round % 19;
                std::memset(memory, pattern, capacity);
                std::this_thread::yield();
                expectPattern(memory, capacity, pattern);
                {
                    std::lock_guard<std::mutex> guard(ownersLock);
                    auto owner = std::find_if(owners.begin(), owners.end(),
                        [=](Owner item) { return item.start == start; });
                    EXPECT_NE(owners.end(), owner);
                    if(owner != owners.end()) owners.erase(owner);
                }
                EXPECT_EQ(capacity, memoryAllocatorFree(&allocator, memory));
            }
        });
    for(auto& worker : workers) worker.join();
    EXPECT_TRUE(owners.empty());
    expectFullyReclaimed();
}

TEST_F(HeapAllocator, SimultaneousBucketFreesReturnCapacityOnlyOnce)
{
    auto memory = memoryAllocatorAllocate(&allocator, 32);
    auto keeper = memoryAllocatorAllocate(&allocator, 32);
    ASSERT_NE(nullptr, memory);
    ASSERT_NE(nullptr, keeper);
    std::atomic<g_size> released{0};
    std::atomic<unsigned> ready{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    for(unsigned i = 0; i < 8; ++i)
        workers.emplace_back([&] {
            ++ready;
            while(!start.load()) std::this_thread::yield();
            released += memoryAllocatorFree(&allocator, memory);
        });
    while(ready.load() != 8) std::this_thread::yield();
    start = true;
    for(auto& worker : workers) worker.join();
    EXPECT_EQ(32U, released.load());
    EXPECT_EQ(32U, memoryAllocatorFree(&allocator, keeper));
    expectFullyReclaimed();
}
}
