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

#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/utils/hashmap_string.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/utils/string.hpp"

static g_mutex entryLock;
static g_hashmap<const char*, g_task_directory_entry*>* entryMap = nullptr;
static g_hashmap<g_tid, const char*>* reverseMap = nullptr;

void taskingDirectoryInitialize()
{
	mutexInitializeGlobal(&entryLock, __func__);
	entryMap = hashmapCreateString<g_task_directory_entry*>(64);
	reverseMap = hashmapCreateNumeric<g_tid, const char*>(64);
}

g_task_directory_entry* _taskingDirectoryGetOrCreateEntry(const char* name)
{
	auto entry = hashmapGet(entryMap, name, (g_task_directory_entry*) nullptr);
	if(!entry)
	{
		entry = (g_task_directory_entry*) heapAllocate(sizeof(g_task_directory_entry));
		entry->task = G_TID_NONE;
		entry->priority = G_SECURITY_LEVEL_APPLICATION;
		waitQueueInitialize(&entry->waitQueue);
		hashmapPut(entryMap, name, entry);
	}
	return entry;
}

void taskingDirectoryWaitForRegister(const char* name, g_tid task)
{
	mutexAcquire(&entryLock);

	auto entry = _taskingDirectoryGetOrCreateEntry(name);
	waitQueueAdd(&entry->waitQueue, task);

	mutexRelease(&entryLock);
}

void taskingDirectoryUnwaitForRegister(const char* name, g_tid task)
{
	mutexAcquire(&entryLock);

	auto entry = _taskingDirectoryGetOrCreateEntry(name);
	waitQueueRemove(&entry->waitQueue, task);

	mutexRelease(&entryLock);
}

bool taskingDirectoryRegister(const char* name, g_tid tid, g_security_level priority)
{
	bool success = false;
	mutexAcquire(&entryLock);

	auto entry = _taskingDirectoryGetOrCreateEntry(name);
	if(priority > entry->priority)
	{
		logInfo("%! tried to override task %s with weaker security level", "taskdir", name);
	}
	else
	{
		success = true;
		entry->task = tid;
		entry->priority = priority;

		auto existingReverseName = hashmapGet(reverseMap, tid, (const char*) nullptr);
		if(existingReverseName)
			heapFree((void*) existingReverseName);
		hashmapPut(reverseMap, tid, (const char*) stringDuplicate(name));

		waitQueueWake(&entry->waitQueue);
	}

	mutexRelease(&entryLock);
	return success;
}

g_tid taskingDirectoryGet(const char* name)
{
	auto entry = hashmapGet(entryMap, name, (g_task_directory_entry*) nullptr);
	return entry ? entry->task : G_TID_NONE;
}

const char* taskingDirectoryGetIdentifier(g_tid tid)
{
	return hashmapGet(reverseMap, tid, (const char*) nullptr);
}
