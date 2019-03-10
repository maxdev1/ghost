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

#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/memory/memory.hpp"

static g_hashmap<g_pid, g_filesystem_process*>* filesystemProcessInfo;

void filesystemProcessInitialize()
{
	filesystemProcessInfo = hashmapCreateNumeric<g_pid, g_filesystem_process*>(128);
}

void filesystemProcessCreate(g_pid pid)
{
	g_filesystem_process* info = (g_filesystem_process*) heapAllocate(sizeof(g_filesystem_process));

	info->nextDescriptor = 3; // after stdin, stdout, stderr
	mutexInitialize(&info->nextDescriptorLock);
	info->descriptors = hashmapCreateNumeric<g_fd, g_file_descriptor>(128);

	hashmapPut(filesystemProcessInfo, pid, info);
}

