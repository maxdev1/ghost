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
#include "kernel/memory/memory_user.hpp"
#include "kernel/system/acpi/platform.hpp"
#include "kernel/system/power/power.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/irq.hpp"
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
	char message[G_PATH_MAX];
	if(!memoryCopyUserString(task, data->message, message, sizeof(message)))
		return;

	if(task->process->object && task->process->object->name)
	{
		int nameBufLen = 64;
		char name[nameBufLen];
		_getBinaryNameWithoutExtension(task, name, nameBufLen);
		logInfo("%! (%i) %s", name, task->id, message);
	}
	else
	{
		logInfo("%! (%i) %s", "unknown", task->id, message);
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
	data->status = task->securityLevel > G_SECURITY_LEVEL_DRIVER
			? G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED : G_VM86_CALL_STATUS_NOT_SUPPORTED;
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

void syscallSystemPower(g_task* task, g_syscall_system_power* data)
{
	data->status = G_POWER_NOT_SUPPORTED;
	if(powerIsReady())
	{
		switch(data->operation)
		{
			case G_POWER_CALL_REQUEST:
				data->status = powerRequest(data->action);
				break;
			case G_POWER_CALL_REGISTER:
				data->status = powerRegister(task, data->role);
				break;
			case G_POWER_CALL_ACKNOWLEDGE:
				data->status = powerAcknowledge(task, data->request, data->ready != 0);
				break;
			default:
				data->status = G_POWER_INVALID_ARGUMENT;
				break;
		}
	}
}

static g_irq_status syscallIrqOperation(g_task* task, const g_syscall_irq& request, g_irq_handle& handle)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return G_IRQ_NOT_PERMITTED;

	switch(request.operation)
	{
		case G_IRQ_BIND:
			return irqInstall(task, request.source, request.line, &handle);
		case G_IRQ_WAIT:
			return irqWait(task, handle, request.timeoutMs);
		case G_IRQ_COMPLETE:
			return irqComplete(task, handle);
		case G_IRQ_MASK:
			return irqMask(task, handle);
		case G_IRQ_RELEASE:
			return irqRelease(task, handle);
		default:
			return G_IRQ_INVALID_ARGUMENT;
	}
}

void syscallIrq(g_task* task, g_syscall_irq* request)
{
	g_irq_handle handle = request->operation == G_IRQ_BIND ? G_IRQ_HANDLE_NONE : request->handle;
	request->status = syscallIrqOperation(task, *request, handle);
	request->handle = handle;
}

void syscallAcpiGetPlatformInfo(g_task* task, g_syscall_acpi_platform_info* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		data->status = G_ACPI_NOT_PERMITTED;
		return;
	}

	data->rsdp = acpiPlatformGetRsdp();
	data->ioapic = ioapicAreAvailable();
	data->status = data->rsdp ? G_ACPI_SUCCESSFUL : G_ACPI_NOT_SUPPORTED;
}

void syscallAcpiMap(g_task* task, g_syscall_acpi_map* data)
{
	data->virtualAddress = 0;

	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		data->status = G_ACPI_NOT_PERMITTED;
		return;
	}

	data->virtualAddress = acpiPlatformMap(task, data->physicalAddress, data->size);
	data->status = data->virtualAddress ? G_ACPI_SUCCESSFUL : G_ACPI_INVALID_ARGUMENT;
}
