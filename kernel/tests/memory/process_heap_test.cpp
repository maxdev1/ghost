#include "support/fixture.hpp"

namespace
{
#include "support/memory/task_pages.hpp"

#include "production/taskingMemoryExtendHeap.inc"

TEST(ProcessHeap, BoundsGrowthPreservesDataOnShrinkAndRollsBackPartialMappings) {
    g_process process{}; process.image.end = (uintptr_t)arena; process.pageSpace = 0x2000;
    g_task task{&process, {}, 2, 7}; uintptr_t result;
    auto change = [&](int32_t amount) {
        bool ok = taskingMemoryExtendHeap(&task, amount, &result);
        EXPECT_TRUE(currentSpace == 0x1000 && process.lock.depth == 0);
        if(!ok) EXPECT_EQ(UINTPTR_MAX, result);
        return ok;
    };
    EXPECT_TRUE(!change(-1) && !allocations && !process.heap.brk);
    EXPECT_TRUE(change(0) && result == (uintptr_t)arena && !allocations && !process.heap.pages);
    EXPECT_TRUE(change(1) && allocations == 1 && process.heap.pages == 1);
    auto first = mappings.at((uintptr_t)arena); ((uint8_t*)first)[0] = 0x42;
    EXPECT_TRUE(change(4096) && process.heap.pages == 2);
    EXPECT_TRUE(change(-1) && process.heap.pages == 1 && frees == 1);
    EXPECT_EQ(0x42, ((uint8_t*)first)[0]);
    EXPECT_TRUE(!change(INT32_MIN) && process.heap.pages == 1);
    EXPECT_TRUE(change(-4096) && !process.heap.pages && frames.empty());
    EXPECT_TRUE(change(8 * 4096) && process.heap.brk == G_USER_MAXIMUM_HEAP_BREAK);
    EXPECT_TRUE(!change(1) && process.heap.pages == 8);
    EXPECT_TRUE(change(-8 * 4096) && frames.empty());
    for(int fail = 0; fail < 3; ++fail) {
        reset(); failPhysical = fail;
        EXPECT_TRUE(!change(3 * 4096) && frames.empty() && allocations == frees && !process.heap.pages);
        reset(); failMapping = fail;
        EXPECT_TRUE(!change(3 * 4096) && frames.empty() && allocations == frees && !process.heap.pages);
        EXPECT_TRUE(mappings.size() == 1 && mappings.begin()->second == 0xbeef000);
    }
}
}
