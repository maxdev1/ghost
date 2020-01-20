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

#include <stdint.h>
#include "ghost.h"

typedef struct {
    uint32_t ti_module;
    uint32_t ti_offset;
} tls_index;

/**
 * This function is called whenever a shared library tries to access a symbol thread-local storage.
 * 
 * The tls_index we get passed here resides in the GOT. The relocations that fill those values are done
 * in the ELF loader for R_386_TLS_DTPMOD32 and R_386_TLS_DTPOFF32.
 * 
 * What we need to do here is ask the kernel for the current threads TLS master image location and then
 * reference to it, according to the offset we get passed.
 */
__attribute__((__regparm__(1))) void* ___tls_get_addr(tls_index* index) {
    g_virtual_address userThreadObjectAddr = (g_virtual_address) g_task_get_tls();
    return (void*) (userThreadObjectAddr + index->ti_offset);
}
