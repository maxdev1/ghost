/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

#define G_DEBUG_THREAD_DUMPING 0
using g_tid = int;
constexpr int G_TID_NONE = -1;
constexpr int G_TASK_STATUS_RUNNING = 1, G_TASK_STATUS_WAITING = 2, G_TASK_STATUS_DEAD = 3;
struct g_processor_state { uint64_t cs = 3; };
struct g_tasking_local;
struct g_task
{
    int id, status = G_TASK_STATUS_RUNNING;
    bool spawnPending = false, terminationRequested = false;
    g_processor_state* state = nullptr;
    struct { int timesScheduled = 0; } statistics{};
    g_mutex lock{};
    uint32_t references = 0;
    const char* waitsFor = nullptr;
    g_tasking_local* assignment = nullptr;
};
#include "production/tasking_local_layout.inc"
void schedulerRequestReschedule(uint32_t) {}
#include "production/taskingPrepareWait.inc"
#include "production/taskingWake.inc"
#include "production/schedulerGetNextTask.inc"
void schedulerAccountTime(g_tasking_local*) {}
#include "production/schedulerSchedule.inc"

class CleanupTest;
CleanupTest* fixture;
bool taskingTryRetireTask(g_task* task);
void taskingDestroyTask(g_task* task);
#include "production/taskingCleanupTasks.inc"

class CleanupTest : public testing::Test
{
protected:
    g_task idle{0}, reaper{1}, first{2}, second{3};
    g_processor_state state;
    g_tasking_local local{};
    std::vector<g_task*> destroyed;
    bool exitDuringDestruction = false;
    bool exitWhileWaitingOnMutex = false;
    bool yieldDuringDestruction = false;

    void SetUp() override
    {
        fixture = this;
        local.scheduling.idleTask = &idle;
        local.scheduling.cleanupTask = &reaper;
        local.scheduling.current = &reaper;
        local.scheduling.preferredTask = G_TID_NONE;
        g_schedule_entry** link = &local.scheduling.list;
        for(auto task : {&reaper, &first, &second})
        {
            auto entry = static_cast<g_schedule_entry*>(heapAllocate(sizeof(g_schedule_entry)));
            *entry = {task, nullptr};
            *link = entry;
            link = &entry->next;
        }
    }

    void TearDown() override
    {
        while(local.scheduling.list)
        {
            auto entry = local.scheduling.list;
            local.scheduling.list = entry->next;
            heapFree(entry);
        }

        EXPECT_TRUE(heapBlocks.empty());
    }

    friend bool taskingTryRetireTask(g_task*);
    friend void taskingDestroyTask(g_task*);
};

bool taskingTryRetireTask(g_task* task)
{
    EXPECT_GT(fixture->local.lock.depth, 0);
    EXPECT_GT(task->lock.depth, 0);
    return task->references == 0;
}

void taskingDestroyTask(g_task* task)
{
    EXPECT_EQ(0, fixture->local.lock.depth);
    EXPECT_EQ(0, task->lock.depth);
    EXPECT_EQ(G_TASK_STATUS_RUNNING, fixture->reaper.status);
    fixture->destroyed.push_back(task);
    if(fixture->yieldDuringDestruction)
    {
        schedulerSchedule(&fixture->local);
        schedulerSchedule(&fixture->local);
        EXPECT_EQ(&fixture->reaper, fixture->local.scheduling.current);
    }

    if(fixture->exitDuringDestruction)
    {
        if(fixture->exitWhileWaitingOnMutex)
            taskingPrepareWait(&fixture->reaper, "mutexAcquire");
        fixture->second.status = G_TASK_STATUS_DEAD;
        schedulerSchedule(&fixture->local);
        if(fixture->exitWhileWaitingOnMutex)
        {
            EXPECT_EQ(G_TASK_STATUS_WAITING, fixture->reaper.status);
            EXPECT_EQ(&fixture->idle, fixture->local.scheduling.current);
            taskingWake(&fixture->reaper); // The mutex backoff expires.
            schedulerSchedule(&fixture->local);
            EXPECT_EQ(&fixture->reaper, fixture->local.scheduling.current);
        }
    }
}

TEST_F(CleanupTest, SleepsAfterReclaimingTasksWithoutATimer)
{
    first.status = G_TASK_STATUS_DEAD;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first}), destroyed);
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);

    schedulerSchedule(&local);

    EXPECT_EQ(&second, local.scheduling.current);
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);
}

TEST_F(CleanupTest, ExitAfterIdlePreparationWakesReaper)
{
    taskingCleanupTasks(&local, &reaper);
    first.status = G_TASK_STATUS_DEAD;
    schedulerSchedule(&local);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);

    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first}), destroyed);
}

TEST_F(CleanupTest, DestructionCanYieldWithoutAnotherExit)
{
    first.status = G_TASK_STATUS_DEAD;
    yieldDuringDestruction = true;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first}), destroyed);
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);
}

TEST_F(CleanupTest, ExitDuringDestructionPreservesWakeForNextPass)
{
    first.status = G_TASK_STATUS_DEAD;
    exitDuringDestruction = true;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);

    exitDuringDestruction = false;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first, &second}), destroyed);
}

TEST_F(CleanupTest, ExitDuringMutexBackoffLeavesReaperAsleepAndPreservesNextPass)
{
    first.status = G_TASK_STATUS_DEAD;
    exitDuringDestruction = true;
    exitWhileWaitingOnMutex = true;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);
    exitDuringDestruction = false;
    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first, &second}), destroyed);
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);
}

TEST_F(CleanupTest, LookupPinDefersReclamationUntilReleased)
{
    first.status = G_TASK_STATUS_DEAD;
    first.references = 1;
    taskingCleanupTasks(&local, &reaper);
    schedulerSchedule(&local);

    EXPECT_TRUE(destroyed.empty());
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);

    first.references = 0;
    schedulerSchedule(&local);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);

    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first}), destroyed);
}

TEST_F(CleanupTest, SchedulerTerminationWakesReaper)
{
    taskingCleanupTasks(&local, &reaper);
    first.terminationRequested = true;
    first.state = &state;
    schedulerSchedule(&local);

    EXPECT_EQ(G_TASK_STATUS_DEAD, first.status);
    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);

    taskingCleanupTasks(&local, &reaper);

    EXPECT_EQ((std::vector<g_task*>{&first}), destroyed);
}
}
