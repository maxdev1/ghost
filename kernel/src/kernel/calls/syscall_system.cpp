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
#include "kernel/memory/memory.hpp"
#include "kernel/system/acpi/platform.hpp"
#include "kernel/system/power/power.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/tasking/elf/elf_object.hpp"
#include "kernel/utils/string.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/boot/limine.hpp"

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

void syscallOpenLogPipe(g_task* task, g_syscall_open_log_pipe* data)
{
	g_fs_node* node;
	if(filesystemCreatePipe(false, &node) != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to open kernel log read pipe", "log");
		data->fd = G_FD_NONE;
		return;
	}

	g_file_flag_mode readFlags = G_FILE_FLAG_MODE_READ;
	g_fs_open_status readOpen = filesystemOpenNodeFd(node, readFlags, task->process->id, &data->fd);
	if(readOpen != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to open read end of kernel log pipe %i for task %i with status %i", "filesystem", node->id,
		        task->id, readOpen);
		data->fd = G_FD_NONE;
		return;
	}

	loggerEnablePipe(node);
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

	taskingWaitIf([&]()
	{
		if(!requestsWaitForIrq(data->irq, task))
			return false;
		if(data->timeout)
			clockWaitForTime(task->id, clockGetLocal()->time + data->timeout);
		return true;
	}, [&]()
	{
		requestsClearWait(data->irq, task);
		if(data->timeout)
			clockUnwaitForTime(task->id);
	});
}

void syscallGetEfiFramebuffer(g_task* task, g_syscall_get_efi_framebuffer* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return;

	auto framebuffer = limineGetFramebuffer();
	g_address framebufferVirtual = (g_address) framebuffer->address;
	data->address = pagingVirtualToPhysical(framebufferVirtual);
	data->width = framebuffer->width;
	data->height = framebuffer->height;
	data->bpp = framebuffer->bpp;
	data->pitch = framebuffer->pitch;
}

void syscallSystemPower(g_task* task, g_syscall_system_power* output)
{
	g_syscall_system_power data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;

	data.status = G_POWER_NOT_SUPPORTED;
	if(powerIsReady())
	{
		switch(data.operation)
		{
			case G_POWER_CALL_REQUEST:
				data.status = powerRequest(data.action);
				break;
			case G_POWER_CALL_REGISTER:
				data.status = powerRegister(task, data.role);
				break;
			case G_POWER_CALL_ACKNOWLEDGE:
				data.status = powerAcknowledge(task, data.request, data.ready != 0);
				break;
			default:
				data.status = G_POWER_INVALID_ARGUMENT;
				break;
		}
	}

	memoryCopyToUser(task, (g_address) output, &data, sizeof(data));
}

void syscallAcpiPlatform(g_task* task, g_syscall_acpi_platform* output)
{
	g_syscall_acpi_platform data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;

	data.status = G_ACPI_NOT_PERMITTED;
	if(task->securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		switch(data.operation)
		{
			case G_ACPI_PLATFORM_INFO:
				data.address = acpiPlatformGetRsdp();
				data.ioapic = ioapicAreAvailable();
				data.status = data.address ? G_ACPI_SUCCESSFUL : G_ACPI_NOT_SUPPORTED;
				break;
			case G_ACPI_PLATFORM_MAP:
				data.address = acpiPlatformMap(task, data.address, data.size);
				data.status = data.address ? G_ACPI_SUCCESSFUL : G_ACPI_INVALID_ARGUMENT;
				break;
			case G_ACPI_PLATFORM_IRQ_WAIT:
				data.status = acpiPlatformWait(task, data.irq);
				break;
			case G_ACPI_PLATFORM_IRQ_INSTALL:
				data.status = acpiPlatformInstallIrq(task, data.irq);
				break;
			case G_ACPI_PLATFORM_IRQ_REARM:
				data.status = acpiPlatformRearmIrq(task, data.irq);
				break;
			case G_ACPI_PLATFORM_IRQ_REMOVE:
				data.status = acpiPlatformRemoveIrq(task, data.irq);
				break;
			case G_ACPI_PLATFORM_IRQ_MASK:
				data.status = acpiPlatformMaskIrq(task, data.irq);
				break;
			default:
				data.status = G_ACPI_INVALID_ARGUMENT;
				break;
		}
	}
	memoryCopyToUser(task, (g_address) output, &data, sizeof(data));
}
