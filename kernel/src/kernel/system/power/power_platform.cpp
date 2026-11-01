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


#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/power/acpi_power.hpp"
#include "kernel/system/power/power.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"

static bool powerStopRequested;
static uint32_t powerStoppedProcessors;

static void powerSleep(uint64_t milliseconds)
{
	auto task = taskingGetCurrentTask();
	taskingWait(task, __func__,
				[task, milliseconds]() { clockWaitForTime(task->id, clockGetLocal()->time + milliseconds); });
}

void powerProcessorCheckpoint()
{
	if(!memoryAtomicLoad(&powerStopRequested, G_MEMORY_ORDER_ACQUIRE) || processorGetCurrentId() == 0)
		return;

	memoryAtomicAddFetch(&powerStoppedProcessors, 1, G_MEMORY_ORDER_RELEASE);

	while(memoryAtomicLoad(&powerStopRequested, G_MEMORY_ORDER_ACQUIRE))
		asm volatile("pause");

	memoryAtomicSubFetch(&powerStoppedProcessors, 1, G_MEMORY_ORDER_RELEASE);
}

static void powerResumeProcessors()
{
	memoryAtomicStore(&powerStopRequested, false, G_MEMORY_ORDER_RELEASE);

	while(memoryAtomicLoad(&powerStoppedProcessors, G_MEMORY_ORDER_ACQUIRE))
		powerSleep(1);
}

static bool powerStopProcessors()
{
	uint64_t deadline;
	logInfo("%! storage flushed; stopping %i secondary CPU(s)", "power", processorGetNumberOfProcessors() - 1);

	memoryAtomicStore(&powerStopRequested, true, G_MEMORY_ORDER_RELEASE);
	deadline = clockGetLocal()->time + 2000;

	while(memoryAtomicLoad(&powerStoppedProcessors, G_MEMORY_ORDER_ACQUIRE) + 1 < processorGetNumberOfProcessors())
	{
		if(clockGetLocal()->time >= deadline)
		{
			powerResumeProcessors();
			return false;
		}
		powerSleep(1);
	}
	return true;
}

g_power_status powerPlatformCommit(g_power_action action)
{
	if(action == G_POWER_REBOOT)
		powerAcpiTryPciReboot();

	if(!powerStopProcessors())
		return G_POWER_TIMED_OUT;

	logInfo("%! committing %s", "power", action == G_POWER_SHUTDOWN ? "shutdown" : "reboot");

	interruptsDisable();
	asm volatile("wbinvd" ::: "memory");

	powerAcpiPerform(action);
	memoryAtomicStore(&powerStopRequested, false, G_MEMORY_ORDER_RELEASE);

	interruptsEnable();
	powerResumeProcessors();
	return G_POWER_HARDWARE_ERROR;
}
