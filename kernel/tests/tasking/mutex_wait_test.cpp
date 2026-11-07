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

#define mutexAcquire fixtureMutexAcquire
#include "support/fixture.hpp"
#undef mutexAcquire

static constexpr int G_MUTEX_INITIALIZED = 0xfeed;
static constexpr unsigned G_MUTEX_MAX_PAUSES = 1024;
struct g_task
{
    unsigned id = 17;
    struct { bool active = false; } timeoutWait;
} mutexTask;
struct g_tasking_local { struct { int globalLockCount = 0; } locking; } mutexLocal;
static bool mutexInterrupts, mutexExpectedInterrupts, mutexContended;
static unsigned mutexAttempts, mutexYields, mutexFailures = 1, mutexSleeps;
static bool mutexWaiting;
struct g_clock_local { uint64_t time = 10; } mutexClock;
g_clock_local* clockGetLocal() { return &mutexClock; }
uint64_t clockGetMilliseconds() { return mutexClock.time; }
void taskingPrepareWait(g_task* task, const char*)
{
    EXPECT_EQ(&mutexTask, task);
    EXPECT_FALSE(mutexInterrupts);
    EXPECT_FALSE(task->timeoutWait.active);
    mutexWaiting = true;
}
void clockWaitForTime(unsigned task, uint64_t deadline)
{
    EXPECT_EQ(mutexTask.id, task);
    EXPECT_TRUE(mutexWaiting);
    EXPECT_FALSE(mutexInterrupts);
    EXPECT_EQ(mutexClock.time + 1, deadline);
    mutexTask.timeoutWait.active = true;
    ++mutexSleeps;
}
void clockUnwaitForTime(g_task* task)
{
    EXPECT_EQ(&mutexTask, task);
    EXPECT_FALSE(mutexWaiting);
    EXPECT_FALSE(mutexInterrupts);
    task->timeoutWait.active = false;
}
bool interruptsAreEnabled() { return mutexInterrupts; }
void interruptsDisable() { mutexInterrupts = false; }
void interruptsEnable() { mutexInterrupts = true; }
bool systemIsReady() { return true; }
unsigned processorGetCurrentId() { return 3; }
g_task* taskingGetCurrentTask() { return &mutexTask; }
g_tasking_local* taskingGetLocal() { return &mutexLocal; }
void mutexErrorUninitialized(g_mutex*) { FAIL() << "Unexpected uninitialized mutex"; }
bool _mutexTryAcquire(g_mutex*, uint32_t owner, bool hadIF)
{
    EXPECT_FALSE(mutexInterrupts);
    EXPECT_EQ(mutexTask.id, owner);
    EXPECT_EQ(mutexExpectedInterrupts, hadIF);
    return ++mutexAttempts > mutexFailures || !mutexContended;
}

void taskingYield()
{
    ++mutexYields;
    if(mutexWaiting)
    {
        EXPECT_FALSE(mutexInterrupts);
        EXPECT_TRUE(mutexTask.timeoutWait.active);
        mutexWaiting = false;
    }
    else
        EXPECT_EQ(mutexExpectedInterrupts, mutexInterrupts);
}
void pagingPollShootdown() {}
#include "production/mutex_acquire.inc"

TEST(TaskMutexWait, PreservesCallerInterruptsWhileYieldingAndOnAcquisition)
{
    mutexFailures = 1;
    for(bool enabled : {false, true})
    {
        for(bool contended : {false, true})
        {
            mutexInterrupts = mutexExpectedInterrupts = enabled;
            mutexContended = contended;
            mutexAttempts = mutexYields = 0;
            g_mutex mutex{};
            mutex.initialized = G_MUTEX_INITIALIZED;
            mutex.type = G_MUTEX_TYPE_TASK;
            mutexAcquire(&mutex);

            EXPECT_EQ(enabled, mutexInterrupts);
            EXPECT_EQ(contended ? 1U : 0U, mutexYields);
        }
    }
}

TEST(TaskMutexWait, ProlongedContentionParksWithoutOverwritingAnExistingTimeout)
{
    for(bool enabled : {false, true})
    {
        for(bool activeTimeout : {false, true})
        {
            mutexInterrupts = mutexExpectedInterrupts = enabled;
            mutexContended = true;
            mutexFailures = 20;
            mutexAttempts = mutexYields = mutexSleeps = 0;
            mutexWaiting = false;
            mutexTask.timeoutWait.active = activeTimeout;
            g_mutex mutex{};
            mutex.initialized = G_MUTEX_INITIALIZED;
            mutex.type = G_MUTEX_TYPE_TASK;
            mutexAcquire(&mutex);

            EXPECT_EQ(enabled, mutexInterrupts);
            EXPECT_EQ(20U, mutexYields);
            EXPECT_EQ(activeTimeout, mutexTask.timeoutWait.active);
            EXPECT_EQ(activeTimeout ? 0U : 12U, mutexSleeps);
        }
    }
    mutexTask.timeoutWait.active = false;
}
