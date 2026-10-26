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

typedef struct
{
	uintptr_t ti_module;
	uintptr_t ti_offset;
} tls_index;

/**
 * This function is called whenever a shared library tries to access a symbol thread-local storage.
 *
 * The tls_index in the GOT contains two pointer-sized words, relocated by the ELF loader.
 * On x86-64 these are R_X86_64_DTPMOD64 and R_X86_64_DTPOFF64. Reading 32-bit
 * words there would mistake the module's upper half for the TLS offset.
 *
 * What we need to do here is ask the kernel for the current threads user-threadlocal object location and then
 * reference to it, according to Ghost's thread-pointer-relative offset.
 */
#if defined(__i386__)
__attribute__((__regparm__(1)))
#endif
void* ___tls_get_addr(tls_index* index)
{
	g_virtual_address userThreadLocalAddr = (g_virtual_address) g_task_get_tls();
	return (void*) (userThreadLocalAddr + index->ti_offset);
}

void* __tls_get_addr(tls_index* index)
{
	return ___tls_get_addr(index);
}
