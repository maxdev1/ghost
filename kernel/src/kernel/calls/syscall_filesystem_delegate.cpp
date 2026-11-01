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

#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/filesystem/filesystem_taskdelegate.hpp"
#include "kernel/memory/memory.hpp"

void syscallFsDelegateRegister(g_task* task, g_syscall_fs_delegate_register* output)
{
	g_syscall_fs_delegate_register data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;

	data.mountpoint_id = G_FS_VIRT_ID_NONE;
	data.transaction_storage = nullptr;
	data.result = G_FS_DELEGATE_NOT_OWNER;

	char name[G_FILENAME_MAX];
	if(task->securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		if(filesystemDelegateCopyName(task, data.name, name))
		{
			auto original = data.name;
			data.name = name;
			data.result = filesystemDelegateRegister(task, &data);
			data.name = original;
		}
		else
			data.result = G_FS_DELEGATE_INVALID_ARGUMENT;
	}

	if(!memoryCopyToUser(task, (g_address) output, &data, sizeof(data)) && data.result == G_FS_DELEGATE_SUCCESSFUL)
		filesystemDelegateUnregister(task, data.mountpoint_id, true);
}

void syscallFsDelegateTransactionComplete(g_task* task, g_syscall_fs_delegate_transaction_complete* output)
{
	g_syscall_fs_delegate_transaction_complete data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;
	data.result = task->securityLevel <= G_SECURITY_LEVEL_DRIVER ? filesystemDelegateComplete(task, data.transaction)
																 : G_FS_DELEGATE_NOT_OWNER;
	memoryCopyToUser(task, (g_address) output, &data, sizeof(data));
}

void syscallFsDelegateNodeCreate(g_task* task, g_syscall_fs_delegate_node_create* output)
{
	g_syscall_fs_delegate_node_create data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;

	data.created_id = G_FS_VIRT_ID_NONE;
	data.result = G_FS_DELEGATE_NOT_OWNER;

	char name[G_FILENAME_MAX];
	if(task->securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		if(filesystemDelegateCopyName(task, data.name, name))
		{
			auto original = data.name;
			data.name = name;
			data.result = filesystemDelegateCreateNode(task, &data);
			data.name = original;
		}
		else
			data.result = G_FS_DELEGATE_INVALID_ARGUMENT;
	}

	memoryCopyToUser(task, (g_address) output, &data, sizeof(data));
}

void syscallFsDelegateUnregister(g_task* task, g_syscall_fs_delegate_unregister* output)
{
	g_syscall_fs_delegate_unregister data;
	if(!memoryCopyFromUser(task, &data, (g_address) output, sizeof(data)))
		return;

	data.result = task->securityLevel <= G_SECURITY_LEVEL_DRIVER
						  ? filesystemDelegateUnregister(task, data.mountpoint_id)
						  : G_FS_DELEGATE_NOT_OWNER;

	memoryCopyToUser(task, (g_address) output, &data, sizeof(data));
}
