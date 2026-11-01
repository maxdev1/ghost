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

#ifndef GHOST_KERNEL_FILESYSTEM_TASKDELEGATE
#define GHOST_KERNEL_FILESYSTEM_TASKDELEGATE

#include "kernel/filesystem/filesystem.hpp"
#include <ghost/filesystem/callstructs.h>

struct g_fs_delegate_pending_node
{
	g_fs_node* node;
	g_fs_node_entry* entry;
	g_fs_delegate_pending_node* next;
};

struct g_fs_delegate_transaction
{
	g_fs_delegate_transaction_storage request;
	g_fs_delegate_transaction_storage result;
	g_fs_node* node;
	g_fs_delegate_pending_node* pending;
	uint8_t* buffer;
	g_tid client;
	bool completed;
	bool cancelled;
};

struct g_fs_task_delegate
{
	g_fs_delegate base;
	g_fs_task_delegate* next;
	g_tid owner;
	g_fs_node* root;
	g_fs_node_entry* mountEntry;
	g_physical_address controlPage;
	g_physical_address ioPage;
	g_virtual_address mapping;
	g_fs_delegate_transaction* transaction;
	g_wait_queue waiters;
	bool online;
};

void filesystemDelegateInitialize();

/**
 * Excludes new delegate transactions and flushes every mounted filesystem.
 * Resume restores normal I/O after a cancelled power operation.
 */
bool filesystemDelegateFlushAll(g_tid coordinator);

void filesystemDelegateResume();

bool filesystemDelegateMountRoot(const char* name);

g_fs_delegate_status filesystemDelegateRegister(g_task* task, g_syscall_fs_delegate_register* data);

g_fs_delegate_status filesystemDelegateComplete(g_task* task, g_fs_transaction_id id);

g_fs_delegate_status filesystemDelegateCreateNode(g_task* task, g_syscall_fs_delegate_node_create* data);

g_fs_delegate_status filesystemDelegateUnregister(g_task* task, g_fs_virt_id mount, bool force = false);

void filesystemDelegateTaskRemoved(g_task* task);

bool filesystemDelegateAvailable(g_fs_node* node);

bool filesystemDelegateMapPages(g_task* task, g_fs_task_delegate* delegate);

void filesystemDelegateReleasePages(g_task* task, g_fs_task_delegate* delegate);

bool filesystemDelegateCopyFromCaller(g_task* task, void* buffer, const void* caller, size_t length);

bool filesystemDelegateCopyToCaller(g_task* task, void* caller, const void* buffer, size_t length);

bool filesystemDelegateCopyName(g_task* task, const char* name, char* copy);

#endif
