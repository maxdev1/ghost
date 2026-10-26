#include "support/fixture.hpp"
#include <mutex>

namespace
{
constexpr int G_TASK_STATUS_DEAD = 3, G_TASK_STATUS_WAITING = 2, G_TASK_STATUS_RUNNING = 1;
using g_tid = int;
struct g_task { g_mutex lock; int status, id, waitersJoin; const char* waitsFor; } tasks[2] = {
    {{}, G_TASK_STATUS_RUNNING, 0, 0, nullptr}, {{}, G_TASK_STATUS_RUNNING, 1, 0, nullptr}};
std::mutex taskLocks[2]; thread_local int held;
void acquire(g_mutex* m) {
    EXPECT_EQ(0, held); taskLocks[m == &tasks[0].lock ? 0 : 1].lock(); ++held;
}
void release(g_mutex* m) { --held; taskLocks[m == &tasks[0].lock ? 0 : 1].unlock(); }
#define mutexAcquire acquire
#define mutexRelease release
std::atomic<int> pins{0};
g_task* taskingAcquireTask(int id) { ++pins; return &tasks[id]; }
void taskingReleaseTask(g_task* task) { if(task) --pins; }
void waitQueueAdd(int* queue, int) { EXPECT_EQ(1, held); ++*queue; }
#include "production/taskingPrepareWait.inc"
#include "production/taskingWake.inc"
#include "production/taskingWaitForExit.inc"

TEST(TaskJoin, ReciprocalJoinsNeverNestTaskLocks) {
    for(int i = 0; i < 100; ++i) {
        std::thread a([] { EXPECT_TRUE(taskingWaitForExit(1, &tasks[0])); });
        std::thread b([] { EXPECT_TRUE(taskingWaitForExit(0, &tasks[1])); });
        a.join(); b.join();
        EXPECT_EQ(i + 1, tasks[0].waitersJoin);
        EXPECT_EQ(i + 1, tasks[1].waitersJoin);
    }
    EXPECT_FALSE(taskingWaitForExit(0, &tasks[0]));
    EXPECT_EQ(100, tasks[0].waitersJoin);
    tasks[1].status = G_TASK_STATUS_DEAD;
    EXPECT_TRUE(!taskingWaitForExit(1, &tasks[0]) && tasks[0].status == G_TASK_STATUS_RUNNING);
    EXPECT_EQ(0, pins.load());
}
}
