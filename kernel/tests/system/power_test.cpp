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

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>
#define GHOST_API_STDINT
#include "../../../../libapi/inc/ghost/system/power.h"
#include "../../../../libapi/inc/ghost/syscall/definitions.h"

using g_tid = int32_t;
using g_address = uintptr_t;
using g_virtual_address = uintptr_t;
using g_physical_address = uintptr_t;
constexpr int G_TID_NONE = -1, G_SECURITY_LEVEL_KERNEL = 0, G_SECURITY_LEVEL_DRIVER = 1;
constexpr int G_SECURITY_LEVEL_APPLICATION = 2, G_TASK_TYPE_VITAL = 2;
constexpr int G_MESSAGE_TRANSACTION_NONE = -1, G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1;
struct g_interrupt_guard {};
struct g_mutex {};
void mutexInitializeGlobal(g_mutex*, const char*) {}
void mutexAcquire(g_mutex*) {}
void mutexRelease(g_mutex*) {}
struct g_mutex_guard { explicit g_mutex_guard(g_mutex*) {} };
struct g_process {};
struct g_task { g_tid id; int securityLevel; bool terminationRequested = false; int type = 0; uint32_t powerCalls = 0; };
struct g_clock_local { uint64_t time = 0; } testClock;
static g_task application{10, 2}, storage{20, 1}, foreign{30, 2};
static g_task* current = &application;
static std::map<g_tid, g_task*> tasks;
static std::function<void()> onWait;
static std::function<void(g_tid, const g_power_event&)> onMessage;
static std::vector<std::string> steps;
static bool firmwareSupported, flushSuccessful, firmwarePrepared, copiesValid, firmwareWasPrepared;
g_clock_local* clockGetLocal() { return &testClock; }
uint64_t clockGetMilliseconds() { return clockGetLocal()->time; }
g_task* taskingGetCurrentTask() { return current; }
g_task* taskingAcquireTask(g_tid id) { return tasks.count(id) ? tasks[id] : nullptr; }
void taskingReleaseTask(g_task*) {}
void clockWaitForTime(g_tid, uint64_t until) { testClock.time = until; }
template<class F> void taskingWait(g_task*, const char*, F setup) { setup(); if(onWait) onWait(); }
void taskingPrepareWait(g_task*, const char*) {}
void clockUnwaitForTime(g_task*) {}
void taskingYield() { if(onWait) onWait(); }
void taskingWake(g_task*) {}
g_process* taskingCreateProcess(int) { return nullptr; }
g_task* taskingCreateTask(uintptr_t, g_process*, int) { return nullptr; }
void taskingAssignOnCore(int, g_task*) {}
void acpiPlatformInitialize(uint64_t) {}
void powerAcpiInitialize() {}
bool powerAcpiSupported(g_power_action) { return firmwareSupported; }
bool powerAcpiPrepare(g_power_action) { firmwareWasPrepared = true; steps.push_back("firmware"); return firmwarePrepared; }
void powerAcpiCancel(g_power_action) { if(firmwareWasPrepared) steps.push_back("restore-firmware"); firmwareWasPrepared = false; }
g_power_status powerPlatformCommit(g_power_action) { steps.push_back("commit"); return G_POWER_HARDWARE_ERROR; }
bool filesystemDelegateFlushAll(g_tid) { steps.push_back("flush"); return flushSuccessful; }
void filesystemDelegateResume() { steps.push_back("resume"); }
void logInfo(const char*, ...) {}
void logWarn(const char*, ...) {}
bool memoryCopyFromUser(g_task*, void* target, g_address source, size_t size) {
    if(!copiesValid) return false; std::memcpy(target, reinterpret_cast<void*>(source), size); return true;
}

bool memoryCopyToUser(g_task*, g_address target, const void* source, size_t size) {
    if(!copiesValid) return false; std::memcpy(reinterpret_cast<void*>(target), source, size); return true;
}

int messageQueueSend(int, g_tid receiver, void* data, size_t, int) {
    auto event = static_cast<g_power_event*>(data);
    if(onMessage) onMessage(receiver, *event);
    return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

#include "production/syscall_classification.inc"
#include "production/power_coordinator.inc"
#include "production/power_syscall.inc"

#include "support/memory_user.hpp"
#include "support/syscall_dispatch.hpp"

class Power : public testing::Test {
protected:
    g_power_pending request{G_POWER_SHUTDOWN, G_POWER_HARDWARE_ERROR, false};
    void SetUp() override {
        powerReady = true; powerPending = nullptr; powerPhase = powerActiveCalls = 0; powerGeneration = 7;
        for(auto& participant : powerParticipants) participant = {G_TID_NONE, 0, 0, false};
        application = {10, 2}; storage = {20, 1}; foreign = {30, 2}; current = &application;
        tasks = {{10, &application}, {20, &storage}, {30, &foreign}};
        testClock.time = 0; onWait = {}; onMessage = {}; steps.clear();
        firmwareSupported = flushSuccessful = firmwarePrepared = copiesValid = true; firmwareWasPrepared = false;
    }

    void begin() { powerPending = &request; powerPhase = 1; }
    void approveWhenWaiting() {
        onWait = [] { for(auto& entry : powerParticipants) if(entry.notified) entry.response = 1; };
    }
};

TEST_F(Power, StorageRegistrationRequiresDriverPrivilegeAndAcknowledgementsBelongToTheirEndpoint) {
    EXPECT_EQ(G_POWER_NOT_PERMITTED, powerRegister(&application, G_POWER_STORAGE));
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&application, G_POWER_APPLICATION));
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&storage, G_POWER_STORAGE));

    begin();

    EXPECT_EQ(G_POWER_BUSY, powerRegister(&foreign, G_POWER_APPLICATION));
    EXPECT_EQ(G_POWER_INVALID_ARGUMENT, powerAcknowledge(&application, 6, true));
    EXPECT_EQ(G_POWER_NOT_PERMITTED, powerAcknowledge(&foreign, 7, true));
    EXPECT_EQ(G_POWER_NOT_PERMITTED, powerAcknowledge(&application, 7, true));
    for(auto& entry : powerParticipants) if(entry.task == 10) entry.notified = true;

    EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&application, 7, false));
    EXPECT_EQ(G_POWER_NOT_PERMITTED, powerAcknowledge(&application, 7, true));
}

TEST_F(Power, ApplicationVetoAndTimeoutNeverReachStorageOrHardware) {
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&application, G_POWER_APPLICATION));

    begin();
    onMessage = [](g_tid, const g_power_event&) { EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&application, 7, false)); };

    EXPECT_EQ(G_POWER_CANCELLED, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_TRUE(steps.empty());

    onMessage = {};

    EXPECT_EQ(G_POWER_TIMED_OUT, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_TRUE(steps.empty());
}

TEST_F(Power, OrdersFilesystemFlushBeforeStorageQuiescenceAndFirmwareCommit) {
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&storage, G_POWER_STORAGE));

    begin(); approveWhenWaiting();
    onMessage = [](g_tid, const g_power_event& event) {
        if(event.type == G_POWER_EVENT_PREPARE) steps.push_back("storage");
    };

    EXPECT_EQ(G_POWER_HARDWARE_ERROR, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_EQ((std::vector<std::string>{"flush", "storage", "firmware", "commit"}), steps);
}

TEST_F(Power, FlushFailureAndDeadStorageNeverCommit) {
    begin(); flushSuccessful = false;

    EXPECT_EQ(G_POWER_IO_ERROR, powerExecute(G_POWER_REBOOT));
    EXPECT_EQ((std::vector<std::string>{"flush"}), steps);

    steps.clear(); flushSuccessful = true; powerPending = nullptr;

    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&storage, G_POWER_STORAGE));

    begin(); storage.terminationRequested = true;

    EXPECT_EQ(G_POWER_IO_ERROR, powerExecute(G_POWER_REBOOT));
    EXPECT_EQ((std::vector<std::string>{"flush"}), steps);
}

TEST_F(Power, DrainTimeoutAndUnsupportedFirmwareNeverCommit) {
    begin(); powerActiveCalls = 1;

    EXPECT_EQ(G_POWER_TIMED_OUT, powerExecute(G_POWER_REBOOT));
    EXPECT_TRUE(steps.empty());

    firmwareSupported = false;

    EXPECT_EQ(G_POWER_NOT_SUPPORTED, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_TRUE(steps.empty());
}

TEST_F(Power, StorageAcknowledgementBlocksUntilCancellationAndFinishRestoresService) {
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&storage, G_POWER_STORAGE)); begin();
    for(auto& entry : powerParticipants) if(entry.task == 20) entry.notified = true;
    onWait = [&] { powerFinish(&request, G_POWER_IO_ERROR); };

    EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&storage, 7, true));
    EXPECT_TRUE(request.completed); EXPECT_EQ(G_POWER_IO_ERROR, request.status);
    EXPECT_EQ(nullptr, powerPending); EXPECT_EQ(0U, powerPhase);
    EXPECT_EQ((std::vector<std::string>{"resume"}), steps);
}

TEST_F(Power, MutationBarrierDrainsExistingCallsAndResumesBlockedCallsAfterCancellation) {
    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_FS_WRITE)); EXPECT_EQ(1U, powerActiveCalls);

    powerSyscallLeave(true); EXPECT_EQ(0U, powerActiveCalls);

    EXPECT_FALSE(powerSyscallEnter(G_SYSCALL_MESSAGE_RECEIVE));

    begin(); powerPhase = 2;
    onWait = [&] { powerFinish(&request, G_POWER_CANCELLED); };

    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_FS_OPEN)); EXPECT_EQ(1U, powerActiveCalls);

    powerSyscallLeave(true);
}

TEST_F(Power, ValidatesSyscallArgumentsAndRejectsConcurrentRequests) {
    begin(); EXPECT_EQ(G_POWER_BUSY, powerRequest(G_POWER_SHUTDOWN));

    EXPECT_EQ(G_POWER_INVALID_ARGUMENT, powerRequest(99));

    g_syscall_system_power data{}; data.operation = G_POWER_CALL_REGISTER; data.role = G_POWER_STORAGE;
    syscallTestInvoke(&application, (g_syscall_handler) syscallSystemPower, &data, sizeof(g_syscall_system_power)); EXPECT_EQ(G_POWER_NOT_PERMITTED, data.status);

    data.operation = 255; syscallTestInvoke(&application, (g_syscall_handler) syscallSystemPower, &data, sizeof(g_syscall_system_power)); EXPECT_EQ(G_POWER_INVALID_ARGUMENT, data.status);

    copiesValid = false; data.status = 77; syscallTestInvoke(&application, (g_syscall_handler) syscallSystemPower, &data, sizeof(g_syscall_system_power)); EXPECT_EQ(77, data.status);
}

TEST_F(Power, TerminationReleasesOutstandingMutationCountsExactlyOnce) {
    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_FS_WRITE));
    EXPECT_EQ(1U, powerActiveCalls);

    powerTaskStopped(&application);

    EXPECT_EQ(0U, powerActiveCalls);

    powerTaskStopped(&application);
    powerSyscallLeave(true);

    EXPECT_EQ(0U, powerActiveCalls);
}

TEST_F(Power, RestoresPciBeforeFirmwareMethodsDuringRollback) {
    g_task platform{40, G_SECURITY_LEVEL_DRIVER}; tasks[40] = &platform;

    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&platform, G_POWER_PLATFORM));

    begin(); firmwareWasPrepared = true;
    for(auto& entry : powerParticipants) if(entry.task == platform.id) entry.notified = true;
    onMessage = [&](g_tid receiver, const g_power_event& event) {
        EXPECT_EQ(platform.id, receiver); EXPECT_EQ(G_POWER_EVENT_CANCEL, event.type);

        steps.push_back("restore-pci");

        EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&platform, event.request, true));
    };

    powerFinish(&request, G_POWER_HARDWARE_ERROR);

    EXPECT_EQ((std::vector<std::string>{"restore-pci", "restore-firmware", "resume"}), steps);
}

TEST_F(Power, SpawnsWaitThroughoutApplicationPreparation) {
    begin();
    bool resumed = false;
    onWait = [&] { resumed = true; powerFinish(&request, G_POWER_CANCELLED); };

    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_SPAWN));
    EXPECT_TRUE(resumed);

    powerSyscallLeave(true);

    EXPECT_EQ(0U, powerActiveCalls);
}

TEST_F(Power, StorageProducersDrainBeforeFilesystemAndBlockServiceQuiescence) {
    g_task producer{40, G_SECURITY_LEVEL_DRIVER}; tasks[40] = &producer;

    EXPECT_EQ(G_POWER_NOT_PERMITTED, powerRegister(&application, G_POWER_STORAGE_PRODUCER));
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&producer, G_POWER_STORAGE_PRODUCER));
    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&storage, G_POWER_STORAGE));

    begin(); approveWhenWaiting();
    onMessage = [&](g_tid receiver, const g_power_event& event) {
        if(event.type != G_POWER_EVENT_PREPARE) return;

        steps.push_back(receiver == producer.id ? "producer" : "storage");
        if(receiver == producer.id) EXPECT_EQ(1U, powerPhase);

        else EXPECT_EQ(2U, powerPhase);
    };

    EXPECT_EQ(G_POWER_HARDWARE_ERROR, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_EQ((std::vector<std::string>{"producer", "flush", "storage", "firmware", "commit"}), steps);
}

TEST_F(Power, FailedOrUnresponsiveStorageProducersPreventFilesystemAndHardwareShutdown) {
    g_task producer{40, G_SECURITY_LEVEL_DRIVER}; tasks[40] = &producer;

    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&producer, G_POWER_STORAGE_PRODUCER));

    begin();
    onMessage = [&](g_tid, const g_power_event& event) {
        EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&producer, event.request, false));
    };

    EXPECT_EQ(G_POWER_IO_ERROR, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_TRUE(steps.empty());

    onMessage = {};

    EXPECT_EQ(G_POWER_TIMED_OUT, powerExecute(G_POWER_SHUTDOWN));
    EXPECT_TRUE(steps.empty());
}

TEST_F(Power, StorageProducerAcknowledgementsKeepTheEndpointAvailableForRollback) {
    g_task producer{40, G_SECURITY_LEVEL_DRIVER}; tasks[40] = &producer;

    ASSERT_EQ(G_POWER_SUCCESSFUL, powerRegister(&producer, G_POWER_STORAGE_PRODUCER));

    begin();
    for(auto& entry : powerParticipants) if(entry.task == producer.id) entry.notified = true;

    EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&producer, 7, true));
    EXPECT_NE(nullptr, powerPending);
    onMessage = [&](g_tid receiver, const g_power_event& event) {
        EXPECT_EQ(producer.id, receiver); EXPECT_EQ(G_POWER_EVENT_CANCEL, event.type);

        steps.push_back("restore-producer");

        EXPECT_EQ(G_POWER_SUCCESSFUL, powerAcknowledge(&producer, event.request, true));
    };

    powerFinish(&request, G_POWER_CANCELLED);

    EXPECT_EQ((std::vector<std::string>{"restore-producer", "resume"}), steps);
    EXPECT_TRUE(request.completed);
}

TEST_F(Power, DriversCanFinishAcceptedWorkWithNewThreadsBeforeTheFinalDrain) {
    current = &storage; begin();
    onWait = [] { FAIL() << "Driver worker creation must remain available during preparation"; };

    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_CREATE_TASK));

    powerSyscallLeave(true);
    powerPhase = 2;
    bool waited = false;
    onWait = [&] { waited = true; powerFinish(&request, G_POWER_CANCELLED); };

    EXPECT_TRUE(powerSyscallEnter(G_SYSCALL_CREATE_TASK));

    powerSyscallLeave(true);

    EXPECT_TRUE(waited);
}
