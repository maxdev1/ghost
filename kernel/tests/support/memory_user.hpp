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

#ifndef GHOST_TEST_MEMORY_USER
#define GHOST_TEST_MEMORY_USER

struct g_user_memory
{
	g_task* task;
	g_address address;
	size_t length;
};

g_user_memory* memoryUserAcquire(g_task* task, g_address address, size_t length, bool writable)
{
	std::vector<uint8_t> bytes(length);
	if(!memoryCopyFromUser(task, bytes.data(), address, length) ||
	   (writable && !memoryCopyToUser(task, address, bytes.data(), length)))
		return nullptr;

	static thread_local g_user_memory buffers[16];
	static thread_local unsigned next;
	auto buffer = &buffers[next++ % 16];
	*buffer = {task, address, length};
	return buffer;
}

bool memoryUserRead(g_user_memory* memory, void* target, size_t length)
{
	return memoryCopyFromUser(memory->task, target, memory->address, length);
}

bool memoryUserWrite(g_user_memory* memory, const void* source, size_t length)
{
	return memoryCopyToUser(memory->task, memory->address, source, length);
}

void* memoryUserBuffer(g_user_memory* memory)
{
	if(!memory)
		return nullptr;
	static thread_local std::vector<uint8_t> bytes;
	bytes.resize(memory->length ? memory->length : 1);
	return bytes.data();
}

#endif
