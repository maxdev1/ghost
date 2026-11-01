/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "kernel/system/power/power.hpp"
#include "kernel/calls/syscall.hpp"
#include "kernel/filesystem/filesystem_taskdelegate.hpp"
#include "kernel/ipc/message_queues.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/acpi/platform.hpp"
#include "kernel/system/power/acpi_power.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include <ghost/syscall/definitions.h>

static constexpr uint32_t G_POWER_MAX_PARTICIPANTS = 128;
static constexpr uint64_t G_POWER_PREPARE_TIMEOUT = 30000;
static constexpr uint32_t G_POWER_PHASE_IDLE = 0;
static constexpr uint32_t G_POWER_PHASE_PREPARING = 1;
static constexpr uint32_t G_POWER_PHASE_DRAINING = 2;
static constexpr uint8_t G_POWER_RESPONSE_PENDING = 0;
static constexpr uint8_t G_POWER_RESPONSE_READY = 1;
static constexpr uint8_t G_POWER_RESPONSE_REJECTED = 2;

struct g_power_participant
{
	g_tid task;
	g_power_role role;
	uint8_t response;
	bool notified;
};

struct g_power_pending
{
	g_power_action action;
	g_power_status status;
	bool completed;
};

static g_mutex powerLock;
static g_power_participant powerParticipants[G_POWER_MAX_PARTICIPANTS];
static g_power_pending* powerPending;
static uint32_t powerGeneration;
static uint32_t powerPhase;
static uint32_t powerActiveCalls;
static bool powerReady;
static bool powerInitialized;
static g_task* powerWorker;

/**
 * Power requests own coordinator-visible state until completion. Defer task
 * cancellation until syscall return so no published stack or drain count dies.
 */
static void powerSleep(uint64_t milliseconds)
{
	g_interrupt_guard interrupts;
	auto task = taskingGetCurrentTask();
	taskingPrepareWait(task, __func__);
	clockWaitForTime(task->id, clockGetLocal()->time + milliseconds);
	taskingYield();
	clockUnwaitForTime(task);
}

static bool powerTaskAlive(g_tid id)
{
	auto task = taskingAcquireTask(id);
	bool alive = task && !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE);
	taskingReleaseTask(task);
	return alive;
}

g_power_status powerRegister(g_task* task, g_power_role role)
{
	if(role > G_POWER_STORAGE_PRODUCER)
		return G_POWER_INVALID_ARGUMENT;
	if(role != G_POWER_APPLICATION && task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return G_POWER_NOT_PERMITTED;
	g_mutex_guard guard(&powerLock);
	if(powerPending)
		return G_POWER_BUSY;
	g_power_participant* available = nullptr;
	for(auto& entry: powerParticipants)
	{
		if(entry.task == task->id)
		{
			entry.role = role;
			return G_POWER_SUCCESSFUL;
		}
		if(entry.task == G_TID_NONE || !powerTaskAlive(entry.task))
			available = &entry;
	}
	if(!available)
		return G_POWER_BUSY;
	*available = {task->id, role, 0, false};
	return G_POWER_SUCCESSFUL;
}

g_power_status powerAcknowledge(g_task* task, uint32_t request, bool ready)
{
	mutexAcquire(&powerLock);
	if(!powerPending || request != powerGeneration)
	{
		mutexRelease(&powerLock);
		return G_POWER_INVALID_ARGUMENT;
	}
	for(auto& entry: powerParticipants)
	{
		if(entry.task != task->id || !entry.notified || entry.response)
			continue;
		memoryAtomicStore(&entry.response, ready ? G_POWER_RESPONSE_READY : G_POWER_RESPONSE_REJECTED,
						  G_MEMORY_ORDER_RELEASE);
		bool storage = entry.role == G_POWER_STORAGE && ready;
		mutexRelease(&powerLock);
		if(storage)
		{
			while(memoryAtomicLoad(&powerPhase, G_MEMORY_ORDER_ACQUIRE) &&
				  memoryAtomicLoad(&powerGeneration, G_MEMORY_ORDER_ACQUIRE) == request)
				powerSleep(1);
		}
		return G_POWER_SUCCESSFUL;
	}
	mutexRelease(&powerLock);
	return G_POWER_NOT_PERMITTED;
}

static g_power_status powerNotifyParticipants(g_power_role role, g_power_action action)
{
	for(auto& entry: powerParticipants)
	{
		if(entry.task == G_TID_NONE || entry.role != role)
			continue;
		if(!powerTaskAlive(entry.task))
		{
			if(role != G_POWER_APPLICATION)
				return G_POWER_IO_ERROR;
			entry.task = G_TID_NONE;
			continue;
		}
		g_power_event event{G_POWER_EVENT_MAGIC, powerGeneration, action, G_POWER_EVENT_PREPARE,
							taskingGetCurrentTask()->id};
		mutexAcquire(&powerLock);
		entry.notified = true;
		entry.response = G_POWER_RESPONSE_PENDING;
		mutexRelease(&powerLock);
		auto sent = messageQueueSend(G_TID_NONE, entry.task, &event, sizeof(event), G_MESSAGE_TRANSACTION_NONE);
		if(sent != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
			return G_POWER_BUSY;
	}
	return G_POWER_SUCCESSFUL;
}

static g_power_status powerWaitForParticipants(g_power_role role, uint64_t deadline)
{
	for(;;)
	{
		bool waiting = false;
		for(auto& entry: powerParticipants)
		{
			if(entry.task == G_TID_NONE || entry.role != role || !entry.notified)
				continue;
			if(!powerTaskAlive(entry.task))
			{
				if(role != G_POWER_APPLICATION)
					return G_POWER_IO_ERROR;
				continue;
			}
			auto response = memoryAtomicLoad(&entry.response, G_MEMORY_ORDER_ACQUIRE);
			if(response == G_POWER_RESPONSE_REJECTED)
				return role != G_POWER_APPLICATION ? G_POWER_IO_ERROR : G_POWER_CANCELLED;
			if(!response)
				waiting = true;
		}
		if(!waiting)
			return G_POWER_SUCCESSFUL;
		if(clockGetLocal()->time >= deadline)
		{
			logWarn("%! preparation timed out for role %i", "power", role);
			return G_POWER_TIMED_OUT;
		}
		powerSleep(5);
	}
}

static g_power_status powerPrepareParticipants(g_power_role role, g_power_action action)
{
	auto deadline = clockGetLocal()->time + G_POWER_PREPARE_TIMEOUT;
	auto status = powerNotifyParticipants(role, action);
	if(status != G_POWER_SUCCESSFUL)
		return status;
	return powerWaitForParticipants(role, deadline);
}

static void powerCancelParticipants(g_power_action action)
{
	for(auto& entry: powerParticipants)
	{
		if(!entry.notified || entry.role == G_POWER_STORAGE)
			continue;
		bool acknowledgeCancel = entry.role == G_POWER_PLATFORM || entry.role == G_POWER_STORAGE_PRODUCER;
		if(acknowledgeCancel)
			memoryAtomicStore(&entry.response, G_POWER_RESPONSE_PENDING, G_MEMORY_ORDER_RELEASE);
		g_power_event event{G_POWER_EVENT_MAGIC, powerGeneration, action, G_POWER_EVENT_CANCEL,
							taskingGetCurrentTask()->id};
		auto deadline = clockGetLocal()->time + G_POWER_PREPARE_TIMEOUT;
		while(messageQueueSend(G_TID_NONE, entry.task, &event, sizeof(event), G_MESSAGE_TRANSACTION_NONE) !=
					  G_MESSAGE_SEND_STATUS_SUCCESSFUL &&
			  powerTaskAlive(entry.task) && clockGetLocal()->time < deadline)
			powerSleep(1);
		if(acknowledgeCancel)
			while(!memoryAtomicLoad(&entry.response, G_MEMORY_ORDER_ACQUIRE) && powerTaskAlive(entry.task) &&
				  clockGetLocal()->time < deadline)
				powerSleep(1);
	}
}

static g_power_status powerDrainCalls()
{
	mutexAcquire(&powerLock);
	memoryAtomicStore(&powerPhase, G_POWER_PHASE_DRAINING, G_MEMORY_ORDER_RELEASE);
	mutexRelease(&powerLock);
	auto deadline = clockGetLocal()->time + G_POWER_PREPARE_TIMEOUT;
	while(memoryAtomicLoad(&powerActiveCalls, G_MEMORY_ORDER_ACQUIRE))
	{
		if(clockGetLocal()->time >= deadline)
		{
			logWarn("%! timed out draining %i active calls", "power", powerActiveCalls);
			return G_POWER_TIMED_OUT;
		}
		powerSleep(5);
	}
	return G_POWER_SUCCESSFUL;
}

static g_power_status powerExecute(g_power_action action)
{
	if(!powerAcpiSupported(action))
		return G_POWER_NOT_SUPPORTED;

	auto status = powerPrepareParticipants(G_POWER_APPLICATION, action);
	if(status != G_POWER_SUCCESSFUL)
		return status;

	logInfo("%! applications prepared", "power");
	status = powerPrepareParticipants(G_POWER_STORAGE_PRODUCER, action);
	if(status != G_POWER_SUCCESSFUL)
		return status;

	logInfo("%! storage producers drained", "power");
	status = powerDrainCalls();
	if(status != G_POWER_SUCCESSFUL)
		return status;

	if(!filesystemDelegateFlushAll(taskingGetCurrentTask()->id))
		return G_POWER_IO_ERROR;
	logInfo("%! filesystems flushed", "power");

	status = powerPrepareParticipants(G_POWER_STORAGE, action);
	if(status != G_POWER_SUCCESSFUL)
		return status;

	logInfo("%! storage services prepared", "power");
	if(!powerAcpiPrepare(action))
		return G_POWER_HARDWARE_ERROR;

	status = powerPrepareParticipants(G_POWER_PLATFORM, action);
	if(status != G_POWER_SUCCESSFUL)
		return status;

	return powerPlatformCommit(action);
}

static void powerFinish(g_power_pending* request, g_power_status status)
{
	powerCancelParticipants(request->action);
	powerAcpiCancel(request->action);
	filesystemDelegateResume();
	mutexAcquire(&powerLock);
	powerPending = nullptr;
	memoryAtomicStore(&powerPhase, G_POWER_PHASE_IDLE, G_MEMORY_ORDER_RELEASE);
	request->status = status;
	memoryAtomicStore(&request->completed, true, G_MEMORY_ORDER_RELEASE);
	mutexRelease(&powerLock);
}

static void powerCoordinator()
{
	powerAcpiInitialize();
	memoryAtomicStore(&powerInitialized, true, G_MEMORY_ORDER_RELEASE);

	for(;;)
	{
		mutexAcquire(&powerLock);
		auto request = powerPending;
		if(!request)
			taskingPrepareWait(powerWorker, __func__);
		mutexRelease(&powerLock);

		if(!request)
		{
			taskingYield();
			continue;
		}

		logInfo("%! preparing %s", "power", request->action == G_POWER_SHUTDOWN ? "shutdown" : "reboot");
		auto status = powerExecute(request->action);
		powerFinish(request, status);
		logWarn("%! cancelled: status %i", "power", status);
	}
}

void powerInitialize(g_physical_address rsdp)
{
	mutexInitializeGlobal(&powerLock, __func__);
	for(auto& entry: powerParticipants)
		entry.task = G_TID_NONE;
	acpiPlatformInitialize(rsdp);
	auto process = taskingCreateProcess(G_SECURITY_LEVEL_KERNEL);
	auto worker =
			taskingCreateTask(reinterpret_cast<g_virtual_address>(powerCoordinator), process, G_SECURITY_LEVEL_KERNEL);
	if(!worker)
		return;
	powerWorker = worker;
	worker->type = G_TASK_TYPE_VITAL;
	taskingAssignOnCore(0, worker);
	memoryAtomicStore(&powerReady, true, G_MEMORY_ORDER_RELEASE);
}

void powerWaitForInitialization()
{
	while(powerReady && !memoryAtomicLoad(&powerInitialized, G_MEMORY_ORDER_ACQUIRE))
		powerSleep(1);
}

bool powerIsReady() { return memoryAtomicLoad(&powerReady, G_MEMORY_ORDER_ACQUIRE); }

g_power_status powerRequest(g_power_action action)
{
	if(action > G_POWER_REBOOT)
		return G_POWER_INVALID_ARGUMENT;

	g_power_pending request{action, G_POWER_HARDWARE_ERROR, false};
	mutexAcquire(&powerLock);
	if(powerPending)
	{
		mutexRelease(&powerLock);
		return G_POWER_BUSY;
	}

	if(memoryAtomicAddFetch(&powerGeneration, 1, G_MEMORY_ORDER_RELEASE) == 0)
		memoryAtomicAddFetch(&powerGeneration, 1, G_MEMORY_ORDER_RELEASE);

	for(auto& entry: powerParticipants)
	{
		entry.notified = false;
		entry.response = G_POWER_RESPONSE_PENDING;
	}
	powerPending = &request;

	taskingWake(powerWorker);
	memoryAtomicStore(&powerPhase, G_POWER_PHASE_PREPARING, G_MEMORY_ORDER_RELEASE);
	mutexRelease(&powerLock);

	while(!memoryAtomicLoad(&request.completed, G_MEMORY_ORDER_ACQUIRE))
		powerSleep(5);

	return request.status;
}

/**
 * Driver worker threads can finish accepted storage work during preparation.
 * The final drain freezes all mutations after producers acknowledge readiness.
 */
bool powerSyscallEnter(uint32_t call)
{
	if(!memoryAtomicLoad(&powerReady, G_MEMORY_ORDER_ACQUIRE))
		return false;

	if(!syscallMutatesState(call))
		return false;

	for(;;)
	{
		mutexAcquire(&powerLock);
		bool startsTask = syscallStartsTask(call) &&
						  (call != G_SYSCALL_CREATE_TASK || taskingGetCurrentTask()->securityLevel > G_SECURITY_LEVEL_DRIVER);
		bool blocked = powerPhase == G_POWER_PHASE_DRAINING ||
					   (powerPhase == G_POWER_PHASE_PREPARING && startsTask);
		if(!blocked)
		{
			++taskingGetCurrentTask()->powerCalls;
			memoryAtomicAddFetch(&powerActiveCalls, 1, G_MEMORY_ORDER_RELEASE);
		}
		mutexRelease(&powerLock);
		if(!blocked)
			return true;
		powerSleep(1);
	}
}

void powerSyscallLeave(bool tracked)
{
	if(tracked && taskingGetCurrentTask()->powerCalls)
	{
		--taskingGetCurrentTask()->powerCalls;
		memoryAtomicSubFetch(&powerActiveCalls, 1, G_MEMORY_ORDER_RELEASE);
	}
}

void powerTaskStopped(g_task* task)
{
	acpiPlatformTaskStopped(task);
	auto calls = task->powerCalls;
	task->powerCalls = 0;
	if(calls)
		memoryAtomicSubFetch(&powerActiveCalls, calls, G_MEMORY_ORDER_RELEASE);
}
