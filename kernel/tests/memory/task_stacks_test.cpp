#include "support/fixture.hpp"

namespace
{
#include "support/memory/task_pages.hpp"

#include "production/tasking_stack.inc"

TEST(TaskStacks, KeepsGuardAndEndUnmappedAndReclaimsFailedPagePublications) {
    int pool;
    noRange = true;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && !allocations);
    reset(); failPhysical = 0;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && !ranges && !allocations);
    reset(); failMapping = 0;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && frames.empty() && !ranges);
    reset();
    auto stack = taskingMemoryCreateStack(&pool, 7, 7, 10);
    EXPECT_TRUE(stack.start == (uintptr_t)arena && stack.end == (uintptr_t)arena + 10 * 4096);
    g_task task{nullptr, stack, 2, 7};
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.start));
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.end));
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.start - 1));
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 4096));
    auto before = allocations;
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 4096) && allocations == before);
    failMapping = mapAttempts;
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 8192));
    EXPECT_TRUE(allocations == before + 1 && frees == 1);
    mappings.erase(stack.start + 8192); // competing publisher's fixture frame
    while(!mappings.empty()) {
        auto it = mappings.begin(); auto p = it->second; mappings.erase(it); memoryPhysicalFree(p);
    }
    EXPECT_TRUE(frames.empty() && allocations == frees);
}
}
