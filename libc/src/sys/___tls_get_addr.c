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

#include <ghost/tasks.h>
#include <ghost/memory.h>

typedef struct g_tls_index
{
	uintptr_t module;
	uintptr_t offset;
} g_tls_index;

/**
 * Ghost relocates the two-word TLS descriptor to a thread-pointer-relative
 * module base and a module-relative offset. This also handles local-dynamic
 * TLS, whose DTPMOD relocation has no symbol and whose offset is link-time data.
 * Pointer-width arithmetic preserves negative offsets and the thread self slot.
 */
#if defined(__i386__)
__attribute__((__regparm__(1)))
#endif
void* ___tls_get_addr(g_tls_index* index)
{
	g_virtual_address userThreadLocalAddr = (g_virtual_address) g_task_get_tls();
	return (void*) (userThreadLocalAddr + index->module + index->offset);
}

void* __tls_get_addr(g_tls_index* index)
{
	return ___tls_get_addr(index);
}
