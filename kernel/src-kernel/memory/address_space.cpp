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

#include <memory/address_space.hpp>
#include <memory/paging.hpp>
#include <memory/memory.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>
#include <tasking/tasking.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>

/**
 * Creates a mapping from the virtualAddress to the physicalAddress. Writes the entries
 * to the recursively mapped directory in the last 4MB of the memory.
 */
bool g_address_space::map(g_virtual_address virtual_addr, g_physical_address physical_addr, uint32_t table_flags, uint32_t page_flags, bool allow_override) {

	// check if addresses are aligned
	if ((virtual_addr & G_PAGE_ALIGN_MASK) || (physical_addr & G_PAGE_ALIGN_MASK)) {
		g_kernel::panic("%! tried to map unaligned addresses: virt %h to phys %h", "addrspace", virtual_addr, physical_addr);
	}

	// calculate table & page indices
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtual_addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtual_addr);

	// get pointers to directory and table
	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = ((g_page_table) G_CONST_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);

	// create table if it does not exist
	if (directory[ti] == 0) {
		g_physical_address new_table_phys = g_pp_allocator::allocate();
		if (new_table_phys == 0) {
			g_kernel::panic("%! no pages left for mapping", "addrspace");
		}

		// insert table
		directory[ti] = new_table_phys | table_flags;

		// empty the created (and mapped) table
		for (uint32_t i = 0; i < 1024; i++) {
			table[i] = 0;
		}
		g_log_debug("%! created table %i", "addrspace", ti);
	} else {
		// this is illegal and an unrecoverable error
		if (table_flags & G_PAGE_TABLE_USERSPACE) {
			if (((directory[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE) == 0) {
				g_kernel::panic("%! tried to map user page in kernel space table, virt %h", "addrspace", virtual_addr);
			}
		}
	}

	// put address into table
	if (table[pi] == 0 || allow_override) {
		table[pi] = physical_addr | page_flags;

		// flush address
		G_INVLPG(virtual_addr);
		return true;
	} else {
		g_thread* failor = g_tasking::lastThread();
		if (failor != 0) {
			const char* ident = failor->getIdentifier();
			if (ident) {
				g_log_info("%! '%s' (%i) tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", ident, failor->id, virtual_addr,
						physical_addr, table[pi]);
			} else {
				g_log_info("%! %i tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", failor->id, virtual_addr, physical_addr,
						table[pi]);
			}
		} else {
			g_log_info("%! unknown tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", virtual_addr, physical_addr, table[pi]);
		}
	}
	return false;
}

/**
 *
 */
void g_address_space::map_to_temporary_mapped_directory(g_page_directory directory, g_virtual_address virtual_addr, g_physical_address physical_addr,
		uint32_t table_flags, uint32_t page_flags, bool allow_override) {

	// check if addresses are aligned
	if ((virtual_addr & G_PAGE_ALIGN_MASK) || (physical_addr & G_PAGE_ALIGN_MASK)) {
		g_kernel::panic("%! tried to map unaligned addresses: virt %h to phys %h", "addrspace", virtual_addr, physical_addr);
	}

	// calculate table & page indices
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtual_addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtual_addr);

	// create table if it does not exist
	if (directory[ti] == 0) {
		g_physical_address new_table_phys = g_pp_allocator::allocate();
		if (new_table_phys == 0) {
			g_kernel::panic("%! no pages left for mapping", "addrspace");
		}

		// temporary map the table and insert it
		g_virtual_address temp_table_addr = g_temporary_paging_util::map(new_table_phys);
		g_page_table table = (g_page_table) temp_table_addr;
		for (uint32_t i = 0; i < 1024; i++) {
			table[i] = 0;
		}
		g_temporary_paging_util::unmap(temp_table_addr);

		// insert table
		directory[ti] = new_table_phys | table_flags;
		g_log_debug("%! created table %i while mapping %h - %h (temp)", "addrspace", ti, virtual_addr, physical_addr);
	}

	// Insert address into table
	g_physical_address table_phys = (directory[ti] & ~G_PAGE_ALIGN_MASK);

	g_virtual_address temp_table_addr = g_temporary_paging_util::map(table_phys);
	g_page_table table = (g_page_table) temp_table_addr;
	if (table[pi] == 0 || allow_override) {
		table[pi] = physical_addr | page_flags;

	} else {
		g_log_warn("%! tried to map area to virtual pd %h that was already mapped, virt %h -> phys %h, table contains %h", "addrspace", directory, virtual_addr,
				physical_addr, table[pi]);
		g_kernel::panic("%! duplicate mapping", "addrspace");
	}
	g_temporary_paging_util::unmap(temp_table_addr);
}

/**
 *
 */
void g_address_space::unmap(g_virtual_address virtualAddress) {

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtualAddress);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtualAddress);

	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);

	if (directory[ti] == 0) {
		return;
	}

	// Remove address from table
	if (table[pi] != 0) {
		table[pi] = 0;

		// Flush address
		G_INVLPG(virtualAddress);
	}
}

/**
 * Switches to the given page-directory
 */
void g_address_space::switch_to_space(g_page_directory directory) {

	asm volatile("mov %0, %%cr3":: "b"(directory));

}

/**
 * Returns the currenty page directory
 */
g_page_directory g_address_space::get_current_space() {

	uint32_t directory;
	asm volatile("mov %%cr3, %0" : "=r"(directory));
	return (g_page_directory) directory;
}

/**
 *
 */
g_physical_address g_address_space::virtual_to_physical(g_virtual_address addr) {

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(addr);

	g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);

	if (table == 0) {
		return 0;
	}

	return table[pi] & ~G_PAGE_ALIGN_MASK;
}
