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
#include "shared/logger/logger.hpp"


static g_hashmap<const char*, g_task_directory_entry>* taskDirectory = 0;


void taskingDirectoryInitialize()
{
    taskDirectory = hashmapCreateString<g_task_directory_entry>(64);
}

bool taskingDirectoryRegister(const char* name, g_tid tid, g_security_level priority)
{
    auto entry = hashmapGetEntry(taskDirectory, name);
    if(entry && entry->value.priority > priority)
    {
        logInfo("%! tried to override task %s with weaker security level", "taskdir", name);
        return false;
    }

    g_task_directory_entry dirEntry;
    dirEntry.task = tid;
    dirEntry.priority = priority;
    hashmapPut(taskDirectory, name, dirEntry);
    return true;
}

g_tid taskingDirectoryGet(const char* name)
{
    auto entry = hashmapGetEntry(taskDirectory, name);
    if(entry)
    {
        return entry->value.task;
    }
    return G_TID_NONE;
}
