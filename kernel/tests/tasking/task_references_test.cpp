#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
constexpr int G_TID_NONE = -1, G_TASK_STATUS_DEAD = 3;
struct g_task { int id, status; uint32_t references; };
#include "production/task_reference_hpp.inc"
#include "production/hashmap_hpp.inc"

g_hashmap<int, g_task*>* taskGlobalMap;

#include "production/task_reference_cpp.inc"

TEST(TaskReferences, PinsBlockRetirementAndRejectLateLookups) {
    taskGlobalMap = hashmapCreateNumeric<int, g_task*>(16);
    auto task = static_cast<g_task*>(heapAllocateClear(sizeof(g_task)));
    task->id = 7; task->status = 1;
    hashmapPut(taskGlobalMap, 7, task);
    auto pin = taskingAcquireTask(7);
    EXPECT_TRUE(pin == task && task->references == 1);
    task->status = G_TASK_STATUS_DEAD;
    EXPECT_FALSE(taskingTryRetireTask(task));
    auto late = taskingAcquireTask(7);
    EXPECT_FALSE(late);
    taskingReleaseTask(late);
    EXPECT_EQ(1u, task->references);
    taskingReleaseTask(pin);
    EXPECT_EQ(0u, task->references);
    EXPECT_TRUE(taskingTryRetireTask(task));
    heapFree(task);
    auto gone = taskingAcquireTask(7);
    EXPECT_FALSE(gone);
    taskingReleaseTask(gone);
    hashmapDestroy(taskGlobalMap);
    EXPECT_TRUE(heapBlocks.empty());
}
}
