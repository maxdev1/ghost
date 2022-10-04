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

#ifndef __KERNEL_PAGE_REF_TRACKER__
#define __KERNEL_PAGE_REF_TRACKER__

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "ghost/types.h"

/**
 *
 */
struct g_pp_reference_count_table
{
	int16_t referenceCount[1024];
};

/**
 *
 */
struct g_pp_reference_count_directory
{
	g_pp_reference_count_table* tables[1024];
};

void pageReferenceTrackerInitialize();

/**
 * Increments the number of references on a physical page.
 */
void pageReferenceTrackerIncrement(g_physical_address address);

/**
 * Decrements the number of references on a physical page.
 * 
 * @return the remaining number of references
 */
int16_t pageReferenceTrackerDecrement(g_physical_address address);

#endif
