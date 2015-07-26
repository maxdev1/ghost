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

#include <memory/temporary_paging_util.hpp>
#include <memory/address_space.hpp>
#include <memory/paging.hpp>
#include <memory/constants.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/collections/address_stack.hpp>
#include <kernel.hpp>
#include <logger/logger.hpp>

static g_address_stack addressStack;

/**
 *
 */
void g_temporary_paging_util::initialize() {

	uint32_t start = G_CONST_KERNEL_TEMPORARY_VIRTUAL_RANGES_START;
	uint32_t end = G_CONST_KERNEL_TEMPORARY_VIRTUAL_ADDRESS_RANGES_END;
	g_log_debug("%! initializing with range %h to %h", "vtemp", start, end);

	for (g_virtual_address i = start; i < end; i += G_PAGE_SIZE) {
		addressStack.push(i);
	}
}

/**
 *
 */
g_virtual_address g_temporary_paging_util::map(g_physical_address phys) {
	g_virtual_address virt = addressStack.pop();

	if (virt == 0) {
		g_kernel::panic("%! unable to temporary map physical address %h, no free addresses", "vtemp", phys);
	}

	g_address_space::map(virt, phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	return virt;
}

/**
 *
 */
void g_temporary_paging_util::unmap(g_virtual_address virt) {
	g_address_space::unmap(virt);
	addressStack.push(virt);
}

