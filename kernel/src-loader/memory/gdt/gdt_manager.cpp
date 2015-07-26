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

#include <memory/gdt/gdt_manager.hpp>
#include <memory/gdt/gdt_mounter.hpp>

#include <logger/logger.hpp>

/**
 * 
 */
void g_gdt_manager::initialize(uint32_t usableAddress) {

	uint32_t gdtPointerAddress = usableAddress;
	uint32_t gdtAddress = gdtPointerAddress + sizeof(g_gdt_pointer);

	// Create the GDT pointer
	g_gdt_pointer* gdtPointer = (g_gdt_pointer*) gdtPointerAddress;
	gdtPointer->limit = (sizeof(g_gdt_entry) * 3) - 1;
	gdtPointer->base = gdtAddress;

	// Create the GDT
	g_gdt_entry* gdt = (g_gdt_entry*) gdtAddress;

	// Null descriptor, position 0x00
	g_gdt::createGate(&gdt[0], 0, 0, 0, 0);

	// Kernel code segment descriptor, position 0x08
	g_gdt::createGate(&gdt[1], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_CODE_SEGMENT, 0xCF);

	// Kernel data segment descriptor, position 0x10
	g_gdt::createGate(&gdt[2], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_DATA_SEGMENT, 0xCF);

	// Load the GDT
	g_log_debug("%! descriptor table created at %h", "initgdt", gdtAddress);
	g_log_debug("%! pointer at %h with base %h and limit %h", "initgdt", gdtPointer, gdtPointer->base, gdtPointer->limit);
	_loadGdt(usableAddress);
	g_log_debug("%! initialized", "initgdt");
}
