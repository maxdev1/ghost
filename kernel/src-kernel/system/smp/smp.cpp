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

#include <system/smp/smp.hpp>
#include <system/system.hpp>
#include <logger/logger.hpp>
#include <memory/constants.hpp>
#include <memory/memory.hpp>
#include <system/timing/pit.hpp>
#include <system/interrupts/lapic.hpp>
#include <memory/address_space.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <kernel.hpp>

/**
 *
 */
void g_smp::initialize(g_physical_address initialPageDirectoryPhysical) {

	// Write values to lower memory for use within startup code
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_PAGE_DIRECTORY) = initialPageDirectoryPhysical;
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_START_ADDRESS) = (g_virtual_address) g_kernel::run_ap;
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_COUNTER) = 0;

	g_log_debug("%! initial page directory for APs: %h", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_PAGE_DIRECTORY));
	g_log_debug("%! kernel entry point for APs: %h", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_START_ADDRESS));
	g_log_debug("%! initial AP counter value: %i", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_COUNTER));

	// Create enough stacks for all APs
	g_physical_address* stackArray = (g_physical_address*) G_CONST_SMP_STARTUP_AREA_AP_STACK_ARRAY;
	for (uint32_t i = 0; i < g_system::getNumberOfProcessors(); i++) {

		g_physical_address stackPhysical = g_pp_allocator::allocate();
		if (stackPhysical == 0) {
			g_log_info("%*%! could not allocate physical page for AP stack", 0x0C, "smp");
			return;
		}
		g_virtual_address stackVirtual = g_kernel::virtual_range_pool->allocate(1);
		if (stackPhysical == 0) {
			g_log_info("%*%! could not allocate virtual range for AP stack", 0x0C, "smp");
			return;
		}

		g_address_space::map(stackVirtual, stackPhysical, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);

		g_virtual_address stackTop = (stackVirtual + G_PAGE_SIZE);
		stackArray[i] = stackTop;

		g_log_debug("%! created AP stack (%h) placed at %h", "smp", stackArray[i], &stackArray[i]);
	}

	// Copy start object from ramdisk to lower memory
	const char* ap_startup_location = "system/lib/apstartup.o";
	g_ramdisk_entry* startupObject = g_kernel::ramdisk->findAbsolute(ap_startup_location);
	if (startupObject == 0) {
		g_log_info("%*%! could not initialize due to missing apstartup object at '%s'", 0x0C, "smp", ap_startup_location);
		return;
	}
	g_memory::copy((uint8_t*) G_CONST_SMP_STARTUP_AREA_CODE_START, (uint8_t*) startupObject->data, startupObject->datalength);

	// Start APs
	g_processor* n = g_system::getProcessorList();
	while (n) {
		if (!n->bsp) {
			initialize_core(n);
		}
		n = n->next;
	}
}

/**
 *
 */
void g_smp::initialize_core(g_processor* cpu) {

	// Calculate the vector value for the code start
	uint32_t vectorValue = (G_CONST_SMP_STARTUP_AREA_CODE_START >> 12) & 0xFF;

	// Send INIT
	g_lapic::write(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apic << 24);
	g_lapic::write(APIC_REGISTER_INT_COMMAND_LOW, APIC_ICR_DELMOD_INIT | APIC_ICR_LEVEL_ASSERT);
	g_lapic::wait_for_icr_send();

	// Sleep 10 milliseconds
	g_pit::prepareSleep(10000);
	g_pit::performSleep();

	// Send SIPI
	g_lapic::write(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apic << 24);
	g_lapic::write(APIC_REGISTER_INT_COMMAND_LOW, vectorValue | APIC_ICR_DELMOD_SIPI | APIC_ICR_LEVEL_ASSERT);

	// Sleep 200µs
	g_pit::prepareSleep(200);
	g_pit::performSleep();

	// Send SIPI
	g_lapic::write(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apic << 24);
	g_lapic::write(APIC_REGISTER_INT_COMMAND_LOW, vectorValue | APIC_ICR_DELMOD_SIPI | APIC_ICR_LEVEL_ASSERT);

	// Sleep 200µs
	g_pit::prepareSleep(200);
	g_pit::performSleep();
}
