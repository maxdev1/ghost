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

#include "kernel/memory/paging.hpp"
#include "shared/memory/constants.hpp"
#include "shared/panic.hpp"

g_physical_address pagingVirtualToPhysical(g_virtual_address addr)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(addr);

	g_page_directory directory = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	if(directory[ti] == 0)
		return 0;

	g_page_table table = ((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);
	return table[pi] & ~G_PAGE_ALIGN_MASK;
}
