#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"
#include "production/hashmap_hpp.inc"

using g_tid = int;
constexpr int G_TID_NONE = -1;
struct g_task { int id, spawnParent; bool spawnPending; };
g_hashmap<int, g_task*>* taskGlobalMap;
g_task* taskingAcquireTask(int id) { return hashmapGet(taskGlobalMap, id, static_cast<g_task*>(nullptr)); }
void taskingReleaseTask(g_task*) {}
std::vector<int> requested;
void taskingRequestTermination(g_task* task) { requested.push_back(task->id); }

#include "production/taskingCancelPendingSpawns.inc"

TEST(SpawnCancellation, TargetsOnlyPendingChildrenOfTheExitingParent) {
    taskGlobalMap = hashmapCreateNumeric<int, g_task*>(16);
    g_task children[] = {{9, 42, true}, {8, 42, true}, {7, 43, true}, {3, 42, false}, {1, 42, true}};
    for(auto& child : children) hashmapPut(taskGlobalMap, child.id, &child);
    taskingCancelPendingSpawns(42);
    EXPECT_TRUE((requested == std::vector<int>{1, 8, 9}));
    requested.clear();
    taskingCancelPendingSpawns(99);
    EXPECT_TRUE(requested.empty());
    hashmapDestroy(taskGlobalMap);
    EXPECT_TRUE(heapBlocks.empty());
}
}
