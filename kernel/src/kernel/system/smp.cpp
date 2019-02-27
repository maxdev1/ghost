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

#include "kernel/system/smp.hpp"
#include "kernel/system/interrupts/lapic.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/kernel.hpp"

#include "shared/logger/logger.hpp"

bool smpInitialized = false;

void smpInitialize(g_physical_address initialPageDirectoryPhysical)
{
	// Write values to lower memory for use within startup code
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_PAGE_DIRECTORY) = initialPageDirectoryPhysical;
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_START_ADDRESS) = (g_virtual_address) kernelRunApplicationCore;
	*((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_COUNTER) = 0;

	logDebug("%! initial page directory for APs: %h", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_PAGE_DIRECTORY));
	logDebug("%! kernel entry point for APs: %h", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_START_ADDRESS));
	logDebug("%! initial AP counter value: %i", "smp", *((uint32_t*) G_CONST_SMP_STARTUP_AREA_AP_COUNTER));

	// Create enough stacks for all APs
	g_physical_address* stackArray = (g_physical_address*) G_CONST_SMP_STARTUP_AREA_AP_STACK_ARRAY;
	for(uint32_t i = 0; i < processorGetNumberOfProcessors(); i++)
	{

		g_physical_address stackPhysical = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(stackPhysical == 0)
		{
			logInfo("%*%! could not allocate physical page for AP stack", 0x0C, "smp");
			return;
		}
		g_virtual_address stackVirtual = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		if(stackPhysical == 0)
		{
			logInfo("%*%! could not allocate virtual range for AP stack", 0x0C, "smp");
			return;
		}

		pagingMapPage(stackVirtual, stackPhysical, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);

		g_virtual_address stackTop = (stackVirtual + G_PAGE_SIZE);
		stackArray[i] = stackTop;

		logDebug("%! created AP stack (%h) placed at %h", "smp", stackArray[i], &stackArray[i]);
	}

	// Copy start object from ramdisk to lower memory
	const char* ap_startup_location = "system/lib/apstartup.o";
	g_ramdisk_entry* startupObject = ramdiskFindAbsolute(ap_startup_location);
	if(startupObject == 0)
	{
		logInfo("%*%! could not initialize due to missing apstartup object at '%s'", 0x0C, "smp", ap_startup_location);
		return;
	}
	memoryCopy((uint8_t*) G_CONST_SMP_STARTUP_AREA_CODE_START, (uint8_t*) startupObject->data, startupObject->dataSize);

	smpInitialized = true;

	// Start APs
	g_processor* core = processorGetList();
	while(core)
	{
		if(!core->bsp)
		{
			smpInitializeCore(core);
		}
		core = core->next;
	}
}

void smpInitializeCore(g_processor* cpu)
{
	// Calculate the vector value for the code start
	uint32_t vectorValue = (G_CONST_SMP_STARTUP_AREA_CODE_START >> 12) & 0xFF;

	// Send INIT
	lapicWrite(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apicId << 24);
	lapicWrite(APIC_REGISTER_INT_COMMAND_LOW, APIC_ICR_DELMOD_INIT | APIC_ICR_LEVEL_ASSERT);
	lapicWaitForIcrSend();

	// Sleep 10 milliseconds
	pitPrepareSleep(10000);
	pitPerformSleep();

	// Send SIPI
	lapicWrite(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apicId << 24);
	lapicWrite(APIC_REGISTER_INT_COMMAND_LOW, vectorValue | APIC_ICR_DELMOD_SIPI | APIC_ICR_LEVEL_ASSERT);

	// Sleep 200µs
	pitPrepareSleep(200);
	pitPerformSleep();

	// Send SIPI
	lapicWrite(APIC_REGISTER_INT_COMMAND_HIGH, cpu->apicId << 24);
	lapicWrite(APIC_REGISTER_INT_COMMAND_LOW, vectorValue | APIC_ICR_DELMOD_SIPI | APIC_ICR_LEVEL_ASSERT);

	// Sleep 200µs
	pitPrepareSleep(200);
	pitPerformSleep();
}
