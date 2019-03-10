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

#include "shared/memory/gdt.hpp"

void gdtCreateGate(g_gdt_entry* entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
	entry->baseLow = (base & 0xFFFF);
	entry->baseMiddle = (base >> 16) & 0xFF;
	entry->baseHigh = (base >> 24) & 0xFF;

	entry->limitLow = (limit & 0xFFFF);
	entry->limitHigh = limit >> 16;

	entry->granularity = granularity;

	entry->access = access;
}
