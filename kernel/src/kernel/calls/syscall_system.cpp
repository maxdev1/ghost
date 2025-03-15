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

#include "kernel/calls/syscall_system.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/tasking/elf/elf_object.hpp"
#include "shared/utils/string.hpp"

void _getBinaryNameWithoutExtension(g_task* task, char buf[], int len)
{
	char* p = buf;
	if(stringLength(task->process->object->name) < len)
	{
		char* ip = task->process->object->name;
		while(*ip && *ip != '.')
		{
			*p++ = *ip++;
		}
	}
	*p = 0;
}

void syscallLog(g_task* task, g_syscall_log* data)
{
	if(task->process->object && task->process->object->name)
	{
		int nameBufLen = 64;
		char name[nameBufLen];
		_getBinaryNameWithoutExtension(task, name, nameBufLen);
		logInfo("%! (%i) %s", name, task->id, data->message);
	}
	else
	{
		logInfo("%! (%i) %s", "unknown", task->id, data->message);
	}
}

void syscallSetVideoLog(g_task* task, g_syscall_set_video_log* data)
{
	loggerEnableVideo(data->enabled);
}

void syscallTest(g_task* task, g_syscall_test* data)
{
	data->result = data->test;
}

void syscallCallVm86(g_task* task, g_syscall_call_vm86* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		data->status = G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED;
		return;
	}

	auto registerStore = (g_vm86_registers*) heapAllocate(sizeof(g_vm86_registers));

	g_task* vm86task = taskingCreateTaskVm86(task->process, data->interrupt, data->in, registerStore);
	taskingAssign(taskingGetLocal(), vm86task);

	for(;;)
	{
		if(vm86task == nullptr || vm86task->status == G_TASK_STATUS_DEAD)
		{
			/* VM86 task has finished, copy out registers */
			*data->out = *registerStore;

			heapFree(registerStore);

			data->status = G_VM86_CALL_STATUS_SUCCESSFUL;
			break;
		}

		taskingYield();
	}
}


void syscallIrqCreateRedirect(g_task* task, g_syscall_irq_create_redirect* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return;

	if(!ioapicAreAvailable())
	{
		logWarn("%! failed to register IRQ redirect for interrupt %i, no ioapic available", "call", data->source);
		return;
	}

	ioapicCreateRedirectionEntry(data->source, data->irq, 0);
}

void syscallAwaitIrq(g_task* task, g_syscall_await_irq* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return;

	if(processorGetCurrentId() != 0)
	{
		logWarn("%! awaiting IRQs only possible on core 0", "call");
		return taskingExit();
	}

	requestsSetHandlerTask(data->irq, task->id);

	INTERRUPTS_PAUSE;
	mutexAcquire(&task->lock);
	task->status = G_TASK_STATUS_WAITING;
	task->waitsFor = "irq";
	mutexRelease(&task->lock);

	if(data->timeout)
	{
		clockUnwaitForTime(task->id);
		clockWaitForTime(task->id, clockGetLocal()->time + data->timeout);
	}

	taskingYield();
	INTERRUPTS_RESUME;
}
