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

using g_tid = int;
constexpr g_tid G_TID_NONE = -1;
constexpr int G_TASK_STATUS_RUNNING = 1, G_TASK_STATUS_WAITING = 2, G_TASK_STATUS_DEAD = 3;
constexpr uint8_t G_SCHEDULER_RESCHEDULE_VECTOR = 0xF1;
#define G_DEBUG_THREAD_DUMPING 0
struct g_tasking_local;
struct g_processor_state { uint64_t cs = 0; };
struct g_kernel_threadlocal { uint32_t processor = 0; };
struct g_task
{
    g_mutex lock{};
    g_tid id = 0;
    int status = G_TASK_STATUS_RUNNING;
    g_tasking_local* assignment = nullptr;
    bool spawnReleased = false;
    bool spawnPending = false, terminationRequested = false;
    uint32_t references = 0;
    g_processor_state* state = nullptr;
    struct { uint32_t timesScheduled = 0; } statistics;
    struct { g_kernel_threadlocal* kernelThreadLocal = nullptr; } threadLocal;
};
struct g_process { g_task* main; };

#include "production/tasking_local_layout.inc"

g_tasking_local locals[3];
g_task idleTasks[3];
uint32_t currentProcessor, ipiCount, lastDestination, parkedPolls, powerSleeps;
const uint32_t apicIds[] = {17, 93, 201};
bool ready, trackingPower, retireFinalPin;
g_task* observedTask;
g_task* retireOnUnlock;
bool powerStopRequested;
uint32_t powerStoppedProcessors;
struct g_clock_local { uint64_t time = 10; } powerClock;

bool systemIsReady() { return ready; }
uint32_t processorGetCurrentId() { return currentProcessor; }
uint32_t processorGetNumberOfProcessors() { return 3; }
uint32_t lapicReadId() { return apicIds[currentProcessor]; }
g_tasking_local* taskingGetLocal() { return &locals[currentProcessor]; }
g_tasking_local* taskingGetLocalForProcessor(uint32_t processor)
{
    return processor < 3 ? &locals[processor] : nullptr;
}
g_clock_local* clockGetLocal() { return &powerClock; }
uint64_t clockGetMilliseconds() { return powerClock.time; }
void schedulerPrepareEntry(g_schedule_entry*) {}
void schedulerAccountTime(g_tasking_local*) {}
void lapicSendIpi(uint32_t apicId, uint8_t vector)
{
    EXPECT_EQ(G_SCHEDULER_RESCHEDULE_VECTOR, vector);
    for(auto& local : locals)
        EXPECT_EQ(0, local.lock.depth);
    if(observedTask)
        EXPECT_EQ(0, observedTask->lock.depth);
    if(trackingPower)
        EXPECT_TRUE(memoryAtomicLoad(&powerStopRequested, G_MEMORY_ORDER_ACQUIRE));
    ++ipiCount;
    lastDestination = apicId;
}

void release(g_mutex* lock)
{
    mutexRelease(lock);
    if(retireOnUnlock && lock == &retireOnUnlock->lock)
    {
        g_task* retired = retireOnUnlock;
        if(retireFinalPin)
        {
            EXPECT_EQ(G_TASK_STATUS_DEAD, retired->status);
            EXPECT_EQ(0U, retired->references);
        }
        else
        {
            EXPECT_TRUE(retired->spawnReleased);
            EXPECT_EQ(G_TASK_STATUS_RUNNING, retired->status);
        }
        retireOnUnlock = nullptr;
        observedTask = nullptr;
        delete retired;
    }
}

void powerResumeProcessors() { powerStopRequested = false; }
void powerSleep(uint64_t)
{
    ++powerSleeps;
    powerStoppedProcessors = 2;
}
void pagingPollShootdown()
{
    EXPECT_EQ(1U, powerStoppedProcessors);
    ++parkedPolls;
    powerStopRequested = false;
}

#define mutexRelease release
#include "production/scheduler_reschedule.inc"
#include "production/schedulerGetNextTask.inc"
#include "production/schedulerPreferTask.inc"
#include "production/taskingWake.inc"
#include "production/taskingReleaseTask.inc"
#include "production/schedulerSchedule.inc"
#include "production/taskingReleaseSpawn.inc"
#include "production/taskingAssign.inc"
#include "production/powerStopProcessors.inc"
#include "production/powerProcessorCheckpoint.inc"
#undef mutexRelease

class RescheduleTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ready = false;
        trackingPower = false;
        retireFinalPin = false;
        ipiCount = lastDestination = parkedPolls = powerSleeps = 0;
        powerStopRequested = false;
        powerStoppedProcessors = 0;
        observedTask = retireOnUnlock = nullptr;
        powerClock.time = 10;
        for(currentProcessor = 0; currentProcessor < 3; ++currentProcessor)
        {
            locals[currentProcessor] = {};
            idleTasks[currentProcessor] = {};
            locals[currentProcessor].processor = currentProcessor;
            schedulerInitializeLocal();
            locals[currentProcessor].scheduling.idleTask = &idleTasks[currentProcessor];
            locals[currentProcessor].scheduling.current = &idleTasks[currentProcessor];
        }
        currentProcessor = 0;
        ready = true;
    }

    void TearDown() override
    {
        EXPECT_TRUE(heapBlocks.empty());
    }
};

TEST_F(RescheduleTest, IgnoresEarlyLocalAndInvalidDestinations)
{
    ready = false;
    schedulerRequestReschedule(1);
    ready = true;
    schedulerRequestReschedule(0);
    schedulerRequestReschedule(3);

    EXPECT_EQ(0U, ipiCount);
    EXPECT_FALSE(locals[1].scheduling.reschedulePending);
}

TEST_F(RescheduleTest, CoalescesUntilDestinationAcknowledgesBeforeScanning)
{
    schedulerRequestReschedule(1);
    schedulerRequestReschedule(1);
    EXPECT_EQ(1U, ipiCount);
    EXPECT_EQ(apicIds[1], lastDestination);
    EXPECT_TRUE(locals[1].scheduling.reschedulePending);

    currentProcessor = 1;
    schedulerAcknowledgeReschedule();
    EXPECT_FALSE(locals[1].scheduling.reschedulePending);

    currentProcessor = 0;
    schedulerRequestReschedule(1);
    EXPECT_EQ(2U, ipiCount);
}

TEST_F(RescheduleTest, WakeAfterAcknowledgementQueuesAnotherSchedulingPass)
{
    g_task task;
    task.status = G_TASK_STATUS_WAITING;
    task.assignment = &locals[1];
    observedTask = &task;
    schedulerRequestReschedule(1);
    currentProcessor = 1;
    schedulerAcknowledgeReschedule();

    currentProcessor = 0;
    taskingWake(&task);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, task.status);
    EXPECT_EQ(2U, ipiCount);
    EXPECT_TRUE(locals[1].scheduling.reschedulePending);
    taskingWake(&task);
    task.status = G_TASK_STATUS_DEAD;
    taskingWake(&task);
    taskingWake(nullptr);
    EXPECT_EQ(2U, ipiCount);
}

TEST_F(RescheduleTest, PrefersRemoteTaskWhenDestinationWasIdle)
{
    g_task first, preferred;
    first.id = 11;
    preferred.id = 12;
    preferred.assignment = &locals[1];
    g_schedule_entry second{&preferred, nullptr}, head{&first, &second};
    locals[1].scheduling.list = &head;

    schedulerPreferTask(&preferred);

    EXPECT_EQ(1U, ipiCount);
    EXPECT_EQ(preferred.id, locals[1].scheduling.preferredTask);
    currentProcessor = 1;
    schedulerAcknowledgeReschedule();
    EXPECT_EQ(&second, schedulerGetNextTask(&locals[1]));
    EXPECT_EQ(G_TID_NONE, locals[1].scheduling.preferredTask);
}

TEST_F(RescheduleTest, ReleasedChildCanRetireBeforeNotification)
{
    g_task* task = new g_task;
    task->status = G_TASK_STATUS_WAITING;
    task->assignment = &locals[1];
    g_process process{task};
    observedTask = retireOnUnlock = task;

    taskingReleaseSpawn(&process);

    EXPECT_EQ(nullptr, retireOnUnlock);
    EXPECT_EQ(1U, ipiCount);
    EXPECT_EQ(apicIds[1], lastDestination);
}

TEST_F(RescheduleTest, RunnableAssignmentNotifiesAfterPublishingItsEntry)
{
    g_task task;
    g_kernel_threadlocal threadLocal;
    task.threadLocal.kernelThreadLocal = &threadLocal;
    observedTask = &task;

    taskingAssign(&locals[1], &task);

    ASSERT_NE(nullptr, locals[1].scheduling.list);
    EXPECT_EQ(&task, locals[1].scheduling.list->task);
    EXPECT_EQ(&locals[1], task.assignment);
    EXPECT_EQ(1U, threadLocal.processor);
    EXPECT_EQ(1U, ipiCount);
    heapFree(locals[1].scheduling.list);
    locals[1].scheduling.list = nullptr;
}

TEST_F(RescheduleTest, PowerStopWakesEveryApplicationProcessorAfterPublication)
{
    trackingPower = true;

    EXPECT_TRUE(powerStopProcessors());

    EXPECT_EQ(2U, ipiCount);
    EXPECT_EQ(apicIds[2], lastDestination);
    EXPECT_EQ(1U, powerSleeps);
}

TEST_F(RescheduleTest, SchedulesReaperWokenByTheLastEntryBeforeIdle)
{
    currentProcessor = 1;
    g_task reaper, dead;
    reaper.status = G_TASK_STATUS_WAITING;
    reaper.assignment = &locals[1];
    dead.status = G_TASK_STATUS_DEAD;
    g_schedule_entry tail{&dead, nullptr}, head{&reaper, &tail};
    locals[1].scheduling.list = &head;
    locals[1].scheduling.cleanupTask = &reaper;
    locals[1].scheduling.current = &dead;

    schedulerSchedule(&locals[1]);

    EXPECT_TRUE(locals[1].scheduling.cleanupPending);
    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);
    EXPECT_EQ(&reaper, locals[1].scheduling.current);
    EXPECT_EQ(1U, reaper.statistics.timesScheduled);
    EXPECT_EQ(0U, idleTasks[1].statistics.timesScheduled);
    EXPECT_EQ(0U, ipiCount);
}

TEST_F(RescheduleTest, SchedulesReaperWokenEarlierAfterItsEntryWasScanned)
{
    currentProcessor = 1;
    g_task reaper, dead, waiting;
    reaper.status = waiting.status = G_TASK_STATUS_WAITING;
    reaper.assignment = &locals[1];
    dead.status = G_TASK_STATUS_DEAD;
    g_schedule_entry tail{&waiting, nullptr}, middle{&dead, &tail}, head{&reaper, &middle};
    locals[1].scheduling.list = &head;
    locals[1].scheduling.cleanupTask = &reaper;
    locals[1].scheduling.current = &waiting;

    schedulerSchedule(&locals[1]);

    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);
    EXPECT_EQ(&reaper, locals[1].scheduling.current);
    EXPECT_EQ(1U, reaper.statistics.timesScheduled);
    EXPECT_EQ(0U, idleTasks[1].statistics.timesScheduled);
    EXPECT_EQ(0U, ipiCount);
}

TEST_F(RescheduleTest, FinalDeadPinNotifiesAfterTheTaskCanBeReclaimed)
{
    g_task* task = new g_task;
    task->status = G_TASK_STATUS_DEAD;
    task->assignment = &locals[1];
    task->references = 2;

    taskingReleaseTask(task);

    EXPECT_EQ(1U, task->references);
    EXPECT_EQ(0U, ipiCount);
    observedTask = retireOnUnlock = task;
    retireFinalPin = true;

    taskingReleaseTask(task);

    EXPECT_EQ(nullptr, retireOnUnlock);
    EXPECT_EQ(1U, ipiCount);
    EXPECT_EQ(apicIds[1], lastDestination);
}

TEST_F(RescheduleTest, FinalPinOfALiveTaskDoesNotRequestScheduling)
{
    g_task task;
    task.assignment = &locals[1];
    task.references = 1;

    taskingReleaseTask(&task);
    taskingReleaseTask(nullptr);

    EXPECT_EQ(0U, task.references);
    EXPECT_EQ(0U, ipiCount);
}

TEST_F(RescheduleTest, FinalDeadPinWakesAnOwnerThatAlreadySelectedIdle)
{
    currentProcessor = 1;
    g_task reaper, dead;
    reaper.status = G_TASK_STATUS_WAITING;
    reaper.assignment = dead.assignment = &locals[1];
    dead.status = G_TASK_STATUS_DEAD;
    dead.references = 1;
    g_schedule_entry tail{&dead, nullptr}, head{&reaper, &tail};
    locals[1].scheduling.list = &head;
    locals[1].scheduling.cleanupTask = &reaper;
    locals[1].scheduling.current = &dead;

    schedulerSchedule(&locals[1]);

    ASSERT_EQ(&idleTasks[1], locals[1].scheduling.current);
    EXPECT_EQ(G_TASK_STATUS_WAITING, reaper.status);
    currentProcessor = 0;
    taskingReleaseTask(&dead);
    ASSERT_EQ(1U, ipiCount);
    currentProcessor = 1;
    schedulerAcknowledgeReschedule();
    schedulerSchedule(&locals[1]);

    EXPECT_EQ(&reaper, locals[1].scheduling.current);
    EXPECT_EQ(G_TASK_STATUS_RUNNING, reaper.status);
    EXPECT_EQ(0U, dead.references);
}

TEST_F(RescheduleTest, ParkedProcessorAcknowledgesShootdowns)
{
    currentProcessor = 1;
    powerStopRequested = true;

    powerProcessorCheckpoint();

    EXPECT_EQ(1U, parkedPolls);
    EXPECT_EQ(0U, powerStoppedProcessors);
}
}
