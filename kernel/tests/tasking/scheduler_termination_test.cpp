#include "support/fixture.hpp"

namespace
{
#define G_DEBUG_THREAD_DUMPING 0
constexpr int RUNNING = 1, G_TASK_STATUS_RUNNING = 1, G_TASK_STATUS_WAITING = 2, G_TASK_STATUS_DEAD = 3;
struct g_processor_state { uint64_t cs; };
struct g_task {
    int status; bool spawnPending, terminationRequested; g_processor_state* state;
    struct { int timesScheduled; } statistics;
    g_mutex lock;
    uint32_t references;
};
struct g_schedule_entry { g_task* task; g_schedule_entry* next; };
struct g_tasking_local {
    g_mutex lock;
    struct { g_schedule_entry* list; g_task* current; g_task* idleTask; g_task* cleanupTask; bool cleanupPending; } scheduling;
};
g_schedule_entry* schedulerGetNextTask(g_tasking_local* local) { return local->scheduling.list; }
void schedulerAccountTime(g_tasking_local*) {}

#include "production/taskingWake.inc"
#include "production/schedulerSchedule.inc"

TEST(SchedulerTermination, DefersTerminationAcrossKernelExecutionAndPendingSpawn) {
    g_processor_state state{0};
    g_task task{RUNNING, false, true, &state, {}}, idle{RUNNING, false, false, &state, {}};
    g_schedule_entry entry{&task, nullptr};
    g_tasking_local local;
    local.scheduling = {&entry, &idle, &idle};
    schedulerSchedule(&local);
    EXPECT_TRUE(local.scheduling.current == &task && task.status == RUNNING); // kernel operation finishes
    state.cs = 3;
    schedulerSchedule(&local);
    EXPECT_TRUE(local.scheduling.current == &idle && task.status == G_TASK_STATUS_DEAD);
    task.status = RUNNING; task.spawnPending = true;
    schedulerSchedule(&local);
    EXPECT_TRUE(local.scheduling.current == &task && task.status == RUNNING); // spawn result still borrowed
}
}
