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

#include "kernel/calls/syscall_kernquery.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/string.hpp"

void syscallKernQuery(g_task* task, g_syscall_kernquery* data)
{
	if(data->command == G_KERNQUERY_SYSTEM_RESOURCES)
	{
		g_kernquery_system_resources_data result{};
		uint64_t total, idle, memoryTotal, memoryFree;
		schedulerGetSystemCpuTime(&total, &idle);
		memoryGetPhysicalUsage(&memoryTotal, &memoryFree);

		result.sampled_at = clockGetNanoseconds(processorGetCurrentId());
		result.cpu_total_time = total;
		result.cpu_idle_time = idle;
		result.memory_total = memoryTotal;
		result.memory_free = memoryFree;
		result.processor_count = processorGetNumberOfProcessors();

		data->status = memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result))
			? G_KERNQUERY_STATUS_SUCCESSFUL : G_KERNQUERY_STATUS_ERROR;
	}
	else if(data->command == G_KERNQUERY_TASK_LIST)
	{
		g_kernquery_task_list_data result;
		data->status = G_KERNQUERY_STATUS_ERROR;
		if(!memoryCopyFromUser(task, &result, (g_address) data->buffer, sizeof(result)))
			return;

		auto count = hashmapSize(taskGlobalMap);
		if(count > result.id_buffer_size)
			count = result.id_buffer_size;
		if(count > UINT32_MAX / sizeof(g_tid))
			return;

		auto idsOutput = memoryUserAcquire(task, (g_address) result.id_buffer, count * sizeof(g_tid), true);
		if(!idsOutput)
			return;

		auto ids = static_cast<g_tid*>(heapTryAllocate(count * sizeof(g_tid) + 1));
		if(!ids)
			return;

		result.filled_ids = 0;
		auto iter = hashmapIteratorStart(taskGlobalMap);
		while(result.filled_ids < count && hashmapIteratorHasNext(&iter))
			ids[result.filled_ids++] = hashmapIteratorNext(&iter)->value->id;
		hashmapIteratorEnd(&iter);

		memoryUserWrite(idsOutput, ids, result.filled_ids * sizeof(g_tid));
		heapFree(ids);
		data->status = memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result))
				? G_KERNQUERY_STATUS_SUCCESSFUL : G_KERNQUERY_STATUS_ERROR;
	}
	else if(data->command == G_KERNQUERY_TASK_COUNT)
	{
		g_kernquery_task_count_data result{};
		result.count = hashmapSize(taskGlobalMap);
		data->status = memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result))
				? G_KERNQUERY_STATUS_SUCCESSFUL : G_KERNQUERY_STATUS_ERROR;
	}
	else if(data->command == G_KERNQUERY_TASK_GET_SECURITY)
	{
		g_kernquery_task_security_data result;
		data->status = G_KERNQUERY_STATUS_ERROR;
		if(!memoryCopyFromUser(task, &result, (g_address) data->buffer, sizeof(result)))
			return;

		auto target = taskingAcquireTask(result.id);
		result.found = false;
		result.security_level = UINT8_MAX;
		data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
		if(target)
		{
			mutexAcquire(&target->lock);
			if(target->status != G_TASK_STATUS_DEAD)
			{
				result.found = true;
				result.security_level = target->securityLevel;
				data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
			}

			mutexRelease(&target->lock);
			taskingReleaseTask(target);
		}

		if(!result.found)
			result.id = G_TID_NONE;
		if(!memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result)))
			data->status = G_KERNQUERY_STATUS_ERROR;
	}
	else if(data->command == G_KERNQUERY_TASK_GET_BY_ID || data->command == G_KERNQUERY_PROCESS_GET_BY_ID)
	{
		g_kernquery_task_get_data result{};
		auto out = &result;
		data->status = G_KERNQUERY_STATUS_ERROR;
		if(!memoryCopyFromUser(task, &result.id, (g_address) data->buffer, sizeof(result.id)))
			return;

		auto target = taskingAcquireTask(out->id);
		bool processQuery = data->command == G_KERNQUERY_PROCESS_GET_BY_ID;
		if(!target || (processQuery && target->id != target->process->id))
		{
			data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
			out->found = false;
			out->id = -1;
			taskingReleaseTask(target);
			if(!memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result)))
				data->status = G_KERNQUERY_STATUS_ERROR;
			return;
		}

		mutexAcquire(&target->lock);

		if(target->status == G_TASK_STATUS_DEAD)
		{
			data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
			out->found = false;
			out->id = -1;
		}
		else
		{
			data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
			out->found = true;
			out->id = target->id;
			out->parent = target->process->id;
			out->type = target->type;
		}

		mutexRelease(&target->lock);

		if(out->found)
		{
			mutexAcquire(&target->process->lock);
			if(target->process->environment.executablePath)
				{
					unsigned i = 0;
					while(i + 1 < sizeof(out->source_path) && target->process->environment.executablePath[i])
					{
						out->source_path[i] = target->process->environment.executablePath[i];
						++i;
					}
					out->source_path[i] = 0;
				}
			else
				out->source_path[0] = 0;

			char identifier[sizeof(out->identifier)];
			taskingDirectoryCopyIdentifier(target->id, identifier, sizeof(identifier));
			stringCopy(out->identifier, identifier);

			if(processQuery)
			{
				for(auto entry = target->process->tasks; entry; entry = entry->next)
					schedulerGetCpuTime(entry->task);
				out->cpu_time = memoryAtomicLoad(&target->process->cpuTime, G_MEMORY_ORDER_RELAXED);
			}
			else
				out->cpu_time = schedulerGetCpuTime(target);
			out->memory_used = pagingCountResidentUserBytes(target->process->pageSpace);
			mutexRelease(&target->process->lock);
		}

		taskingReleaseTask(target);
		if(!memoryCopyToUser(task, (g_address) data->buffer, &result, sizeof(result)))
			data->status = G_KERNQUERY_STATUS_ERROR;
	}
	else
	{
		data->status = G_KERNQUERY_STATUS_ERROR;
	}
}
