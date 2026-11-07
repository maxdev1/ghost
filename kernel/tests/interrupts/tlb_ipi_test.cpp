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
#include "production/paging_shootdown_hpp.inc"
#include "production/reschedule_vector.inc"

struct g_processor_state
{
    uint64_t intr, rip, rflags;
};
struct g_task
{
    volatile g_processor_state* state;
};
static g_task currentTask;
struct g_tasking_local { struct { g_task* idleTask; } scheduling; } currentLocal;
g_tasking_local* taskingGetLocal() { return &currentLocal; }
static unsigned ipis, eois, polls, saves, restores, exceptions, checkpoints;
static unsigned refreshes, schedules, acknowledgements, timerConfigurations;
static unsigned finalizations, restoredFinalizations;
static bool resetPreference;

bool pagingHandleShootdownIpi() { ++ipis; return true; }
void pagingPollShootdown() { ++polls; }
void lapicSendEndOfInterrupt() { ++eois; }
g_task* taskingGetCurrentTask() { return &currentTask; }
void taskingSaveState(g_task* task, g_processor_state* state) { ++saves; task->state = state; }
void taskingRestoreState(g_task*) { ++restores; restoredFinalizations = finalizations; }
void exceptionsHandle(g_task*, volatile g_processor_state*) { ++exceptions; }
void syscallHandle(g_task*) {}
void taskingSchedule(bool reset = false) { ++schedules; resetPreference = reset; }
void schedulerAcknowledgeReschedule() { ++acknowledgements; }
void clockRefreshWaiters() { ++refreshes; }
void clockConfigureLocalTimer(bool) { ++timerConfigurations; }
void taskingFinalizeSpawn(g_task* task) { ++finalizations; task->state->intr = 0; }
void clockUpdate() {}
void irqReapRetired() {}
void interruptsSendEndOfInterrupt(uint8_t) { ++eois; }
void irqHandle(g_task*, uint8_t) {}
void powerProcessorCheckpoint() { ++checkpoints; }

#include "production/interrupt_handler.inc"

TEST(TlbIpi, AcknowledgesWithoutChangingTaskStateOrScheduling)
{
    g_processor_state saved{0x80, 0x1234, 0x202};
    g_processor_state interrupted{G_TLB_SHOOTDOWN_VECTOR, 0x5678, 0x202};
    currentTask.state = &saved;
    ipis = eois = polls = saves = restores = exceptions = checkpoints = 0;

    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(&saved, currentTask.state);
    EXPECT_EQ(1U, ipis);
    EXPECT_EQ(1U, eois);
    EXPECT_EQ(0U, polls + saves + restores + exceptions + checkpoints);
}

TEST(TlbIpi, UnrelatedNmisReachExceptionHandling)
{
    g_processor_state interrupted{2, 0x5678, 0x202};
    currentTask.state = &interrupted;
    ipis = eois = polls = saves = restores = exceptions = checkpoints = 0;

    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(0U, ipis + eois + polls + restores);
    EXPECT_EQ(1U, saves);
    EXPECT_EQ(1U, exceptions);
    EXPECT_EQ(1U, checkpoints);
}

TEST(TlbIpi, RescheduleInterruptConsumesItsRequestAndRunsTheScheduler)
{
    g_processor_state interrupted{G_SCHEDULER_RESCHEDULE_VECTOR, 0x5678, 0x202};
    currentTask.state = &interrupted; currentLocal.scheduling.idleTask = nullptr;
    ipis = eois = polls = saves = restores = exceptions = checkpoints = 0;
    refreshes = schedules = acknowledgements = timerConfigurations = 0;
    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(1U, acknowledgements); EXPECT_EQ(1U, refreshes);
    EXPECT_EQ(1U, schedules); EXPECT_EQ(1U, eois);
    EXPECT_EQ(1U, timerConfigurations); EXPECT_EQ(0U, ipis + exceptions);
}
TEST(TlbIpi, DeviceInterruptChecksForRunnableWorkOnAnIdleProcessor)
{
    g_processor_state interrupted{0x21, 0x5678, 0x202};
    currentTask.state = &interrupted; currentLocal.scheduling.idleTask = &currentTask;
    refreshes = schedules = acknowledgements = timerConfigurations = 0;
    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(1U, refreshes); EXPECT_EQ(1U, schedules);
    EXPECT_EQ(0U, acknowledgements); EXPECT_EQ(1U, timerConfigurations);
    currentLocal.scheduling.idleTask = nullptr;
}

TEST(TlbIpi, TimerSchedulingPreservesAnArrivingRemotePreference)
{
    g_processor_state interrupted{0x20, 0x5678, 0x202};
    currentTask.state = &interrupted; currentLocal.scheduling.idleTask = nullptr;
    schedules = 0; resetPreference = true;
    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(1U, schedules); EXPECT_FALSE(resetPreference);
}

TEST(TlbIpi, SpawnDowngradeRestoresNewTlsEvenWhenTheTaskDoesNotChange)
{
    g_processor_state interrupted{0x82, 0x5678, 0x202};
    currentTask.state = &interrupted; currentLocal.scheduling.idleTask = nullptr;
    saves = restores = schedules = finalizations = restoredFinalizations = 0;
    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(1U, finalizations);
    EXPECT_EQ(1U, restores);
    EXPECT_EQ(1U, restoredFinalizations);
    EXPECT_EQ(0U, schedules);
}

TEST(TlbIpi, SameTaskSyscallPreservesInstalledStateAndActiveQuantum)
{
    g_processor_state interrupted{0x80, 0x5678, 0x202};
    currentTask.state = &interrupted; currentLocal.scheduling.idleTask = nullptr;
    saves = restores = schedules = finalizations = 0;
    EXPECT_EQ(&interrupted, _interruptHandler(&interrupted));
    EXPECT_EQ(1U, saves);
    EXPECT_EQ(0U, restores + schedules + finalizations);
}
