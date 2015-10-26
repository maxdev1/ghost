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
#include <memory/gdt/tss.hpp>
#include <memory/constants.hpp>
#include <system/system.hpp>
#include <logger/logger.hpp>
#include <system/smp/global_lock.hpp>

/**
 *
 */
static g_gdt_list_entry** gdtList;

/**
 * 
 */
void g_gdt_manager::prepare() {

	// Create enough space for each core
	uint32_t numCores = g_system::getNumberOfProcessors();
	gdtList = new g_gdt_list_entry*[numCores];

	// Create one GDT per core
	for (uint32_t i = 0; i < numCores; i++) {
		gdtList[i] = new g_gdt_list_entry();
	}

}

/**
 * 
 */
void g_gdt_manager::initialize() {

	// Initialize local GDT
	g_gdt_list_entry* thisGdt = gdtList[g_system::currentProcessorId()];

	// Create the GDT pointer
	thisGdt->ptr.limit = (sizeof(g_gdt_entry) * G_GDT_NUM_ENTRIES) - 1;
	thisGdt->ptr.base = (uint32_t) &thisGdt->entry;

	// Null descriptor, position 0x00
	g_gdt::createGate(&thisGdt->entry[0], 0, 0, 0, 0);

	// Kernel code segment descriptor, position 0x08
	g_gdt::createGate(&thisGdt->entry[1], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_CODE_SEGMENT, 0xCF);

	// Kernel data segment descriptor, position 0x10
	g_gdt::createGate(&thisGdt->entry[2], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_DATA_SEGMENT, 0xCF);

	// User code segment descriptor, position 0x18
	g_gdt::createGate(&thisGdt->entry[3], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_CODE_SEGMENT, 0xCF);

	// User data segment descriptor, position 0x20
	g_gdt::createGate(&thisGdt->entry[4], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);

	// TSS descriptor, position 0x28
	g_gdt::createGate(&thisGdt->entry[5], (uint32_t) &thisGdt->tss, sizeof(g_tss), G_ACCESS_BYTE__TSS_386_SEGMENT, 0x40);
	thisGdt->tss.ss0 = G_GDT_DESCRIPTOR_KERNEL_DATA; // Kernel data segment
	thisGdt->tss.esp0 = 0; // will later be initialized by the scheduler

	// User thread pointer segment 0x30
	g_gdt::createGate(&thisGdt->entry[6], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);

	// Load GDT
	g_log_debug("%! BSP descriptor table lays at %h", "gdt", &thisGdt->entry);
	g_log_debug("%! pointer lays at %h, base %h, limit %h", "gdt", &thisGdt->ptr, thisGdt->ptr.base, thisGdt->ptr.limit);
	_loadGdt((uint32_t) &thisGdt->ptr);
	g_log_debug("%! initialized", "gdt");

	// Load TSS
	g_log_debug("%! descriptor index %h", "tss", G_GDT_DESCRIPTOR_TSS);
	_loadTss(G_GDT_DESCRIPTOR_TSS);
	g_log_debug("%! initialized", "tss");
}

/**
 * Sets the ESP0 in the TSS. This is necessary for the system to know which stack
 * to use when switching from ring 3 to ring 0.
 */
void g_gdt_manager::setTssEsp0(uint32_t esp0) {
	gdtList[g_system::currentProcessorId()]->tss.esp0 = esp0;
}

/**
 *
 */
void g_gdt_manager::setUserThreadAddress(g_virtual_address user_thread_addr) {
	g_gdt_list_entry* list_entry = gdtList[g_system::currentProcessorId()];
	g_gdt::createGate(&list_entry->entry[6], user_thread_addr, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);
}
