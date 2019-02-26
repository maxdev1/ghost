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

#include "kernel/system/interrupts/exceptions.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/memory/paging.hpp"

uint32_t exceptionsGetCR2()
{
	uint32_t addr;
	asm volatile("mov %%cr2, %0" : "=r"(addr));
	return addr;
}

void exceptionsHandle(g_processor_state* state)
{
	logInfo("%*%! no resolution for exception %i, code %i, hanging system", 0x0C, "exception", state->intr, state->error);

	for(;;);
	if(state->intr == 0xE)
	{
		g_virtual_address accessedVirtual = G_PAGE_ALIGN_DOWN(exceptionsGetCR2());
		g_physical_address accessedPhysical = pagingVirtualToPhysical(accessedVirtual);
		logInfo("%# tried to access %h, points to %h", accessedVirtual, accessedPhysical);
	}
	for(;;)
	{
		asm("hlt");
	}
}

