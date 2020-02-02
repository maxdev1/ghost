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

#ifndef __KERNEL_WAIT_RESOLVER__
#define __KERNEL_WAIT_RESOLVER__

#include "ghost/types.h"
#include "kernel/tasking/tasking.hpp"

struct g_fs_node;

struct g_wait_resolver_sleep_data
{
	uint32_t wakeTime;
};

struct g_wait_resolver_atomic_lock_data
{
	uint32_t startTime;
};

struct g_wait_resolver_for_file_data
{
	bool (*waitResolverFromDelegate)(g_task*);
	g_fs_virt_id nodeId;
};

struct g_wait_resolver_join_data
{
	g_tid joinedTaskId;
};

struct g_wait_vm86_data
{
	g_tid vm86TaskId;
	g_vm86_registers* registerStore;
};

bool waitResolverSleep(g_task* task);

bool waitResolverAtomicLock(g_task* task);

bool waitResolverJoin(g_task* task);

bool waitResolverSendMessage(g_task* task);

bool waitResolverReceiveMessage(g_task* task);

bool waitResolverVm86(g_task* task);

#endif
