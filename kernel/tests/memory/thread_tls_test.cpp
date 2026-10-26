#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

#include "support/memory/task_pages.hpp"

#define G_PAGE_UNMANAGED_FLAG 512
#define G_PAGE_PHYSICAL_MASK 0x000ffffffffff000ULL
uint32_t processorGetCurrentId() { return 0; }
void memoryCopy(void* a, const void* b, size_t n) { std::memcpy(a, b, n); }

#include "production/tasking_tls.inc"

TEST(ThreadTls, ReclaimsPartialMappingsWithoutRemovingCompetingPages) {
    int pool; g_process process{};
    process.virtualRangePool = &pool;
    process.tlsMaster = {(uintptr_t)arena + 12 * 4096, 3 * 4096, 0};
    for(int fail = 0; fail < 3; ++fail) {
        reset(); failMapping = fail;
        g_task task{}; task.process = &process;
        EXPECT_FALSE(taskingMemoryInitializeTls(&task));
        EXPECT_EQ((uintptr_t)arena, task.threadLocal.start);
        EXPECT_EQ((uintptr_t)arena + fail * 4096, task.threadLocal.end);
        EXPECT_FALSE(task.threadLocal.userThreadLocal);
        taskingMemoryDestroyTls(&task);
        EXPECT_TRUE(frames.empty() && allocations == frees && ranges == 0 && heapBlocks.empty());
        EXPECT_TRUE(mappings.size() == 1 && mappings.begin()->second == 0xbeef000);
    }
}
}
