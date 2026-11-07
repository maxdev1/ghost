#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
constexpr int G_TASK_STATUS_WAITING = 2, G_TASK_STATUS_RUNNING = 1;
struct g_process { void* spawnArgs; } process;
struct g_tasking_local { uint32_t processor = 1; } parentAssignment;
struct g_task {
    g_process* process;
    g_mutex lock;
    int status, spawnParent;
    bool spawnPending, spawnFinished, spawnReleased;
    g_tasking_local* assignment = nullptr;
} child, parent;
bool parentPresent;
int mode, waits, exits, pins, checks, acquisitions, notifications;
void schedulerRequestReschedule(uint32_t processor) {
    EXPECT_EQ(parentAssignment.processor, processor);
    EXPECT_EQ(0, parent.lock.depth);
    EXPECT_GT(pins, 0);
    ++notifications;
}
g_task* taskingAcquireTask(int id) {
    EXPECT_EQ(42, id);
    if(++acquisitions == 2 && mode == 3)
    {
        heapFree(process.spawnArgs); process.spawnArgs = nullptr;
        child.spawnReleased = true;
        parentPresent = false;
    }
    if(!parentPresent) return nullptr;
    ++pins;
    return &parent;
}
void taskingReleaseTask(g_task* task) { if(task) --pins; }
void taskingExit() { ++exits; child.status = 3; }
void taskingWake(g_task*) {}
void taskingHandleTerminationRequest() { EXPECT_FALSE(child.spawnPending); ++checks; }
template<class F> void taskingWait(g_task*, const char*, const F& prepare) {
    EXPECT_EQ(0, pins); ++waits;
    if(mode == 1) parentPresent = false;
    else {
        heapFree(process.spawnArgs); process.spawnArgs = nullptr;
        child.spawnReleased = true;
    }
    prepare();
}

#include "production/taskingCompleteSpawn.inc"

void run(int scenario) {
    mode = scenario;
    parentPresent = mode != 0;
    parent.status = G_TASK_STATUS_WAITING;
    parent.assignment = &parentAssignment;
    child = g_task{}; child.spawnParent = 42; child.spawnPending = true;
    child.process = &process;
    process.spawnArgs = heapAllocate(32);
    waits = exits = pins = checks = acquisitions = notifications = 0;
    taskingCompleteSpawn(&child);
    EXPECT_TRUE(heapBlocks.empty() && !pins && !process.spawnArgs);
    if(mode < 2) EXPECT_TRUE(exits == 1 && child.status == 3);
    else EXPECT_TRUE(!exits && child.spawnFinished && !child.spawnPending && checks == 1);
    EXPECT_EQ((mode == 0 || mode == 3 ? 0 : 1), waits);
    EXPECT_EQ(mode == 0 ? 0 : 1, notifications);
}
TEST(SpawnHandshake, ReleasesOrphanedChildrenAndCompletesParentAcknowledgement) { run(0); run(1); run(2); }
TEST(SpawnHandshake, ReleasedChildSurvivesParentExitBeforeItsNextLookup) { run(3); }
}
