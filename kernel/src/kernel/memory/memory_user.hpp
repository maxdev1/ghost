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

#ifndef __KERNEL_MEMORY_USER__
#define __KERNEL_MEMORY_USER__

#include <ghost/memory/types.h>

struct g_task;
struct g_user_memory;

/**
 * Copies managed user bytes into a kernel buffer. The task's address space must
 * be active. Missing demand pages are resolved outside the process lock.
 * Returns false for inaccessible memory. Earlier bytes may already be copied.
 */
bool memoryCopyFromUser(g_task* task, void* target, g_address source, size_t length);

/**
 * Copies kernel bytes to writable managed user pages in the active address
 * space. Uses retained syscall pages when available. Returns false for invalid
 * memory, with the same partial-copy behavior as memoryCopyFromUser.
 */
bool memoryCopyToUser(g_task* task, g_address target, const void* source, size_t length);

/**
 * Retains a user range and checks every page's effective permissions before an
 * operation. Writable ranges require write access. May load demand pages, so
 * acquire before taking subsystem locks. Returns null on invalid memory or OOM.
 * The task owns the range until release, syscall return or task destruction.
 */
g_user_memory* memoryUserAcquire(g_task* task, g_address address, size_t length, bool writable);

/**
 * Releases a retained range, its physical-page references and staging buffer.
 * Passing null has no effect.
 */
void memoryUserRelease(g_user_memory* memory);

/**
 * Allocates and returns kernel staging storage for the retained range. Repeated
 * calls return the same storage. Returns null on failure. Release owns storage.
 */
void* memoryUserBuffer(g_user_memory* memory);

/**
 * Releases every retained range owned by a task after a syscall or cancellation.
 */
void memoryUserReleaseAll(g_task* task);

/**
 * Reads bytes from the retained backing pages into a kernel buffer. Returns
 * false if the requested length exceeds the acquired range.
 */
bool memoryUserRead(g_user_memory* memory, void* target, size_t length);

/**
 * Writes kernel bytes to the retained backing pages even if the original user
 * mapping was removed. Returns false if the range is read-only or too short.
 */
bool memoryUserWrite(g_user_memory* memory, const void* source, size_t length);

/**
 * Copies through an existing retained range containing the supplied address.
 * Returns false when no suitable range exists, allowing a checked live copy.
 */
bool memoryUserCopyPinned(g_task* task, g_address address, void* buffer, size_t length, bool toUser);

/**
 * Copies a terminated user string, including its terminator, into a kernel
 * buffer. Returns false on invalid memory or if capacity cannot fit the string.
 */
bool memoryCopyUserString(g_task* task, const char* source, char* target, size_t capacity);

#endif
