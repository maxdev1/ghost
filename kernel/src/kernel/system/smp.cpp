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
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/paging.hpp"

bool smpInitialized = false;

void smpInitialize(g_physical_address initialPageDirectoryPhysical)
{
	// TODO: For all physical allocations below we must make sure that the memory is in 32 bit address range

	// Identity-map lower memory
	for(g_address phys = 0; phys < G_SMP_STARTUP_AREA_END; phys += G_PAGE_SIZE)
	{
		pagingMapPage(phys, phys, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
	}

	// Map it so we can write it here
	g_virtual_address mappedLower = addressRangePoolAllocate(memoryVirtualRangePool,
	                                                         G_PAGE_ALIGN_UP(G_SMP_STARTUP_AREA_END) / G_PAGE_SIZE);
	for(g_address phys = 0; phys < G_SMP_STARTUP_AREA_END; phys += G_PAGE_SIZE)
	{
		pagingMapPage(mappedLower + phys, phys, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
	}

	// Write values to lower memory for use within startup code
	*((g_address*) (mappedLower + G_SMP_STARTUP_AREA_PAGEDIR)) = initialPageDirectoryPhysical;
	*((g_address*) (mappedLower + G_SMP_STARTUP_AREA_AP_ENTRY)) = (g_virtual_address) kernelRunApplicationCore;
	*((g_size*) (mappedLower + G_SMP_STARTUP_AREA_AP_COUNTER)) = 0;

	logDebug("%! initial page directory for APs: %h, %x", "smp",
	        *((g_address*) (mappedLower+G_SMP_STARTUP_AREA_PAGEDIR)),
	        initialPageDirectoryPhysical);
	logDebug("%! kernel entry point for APs: %h", "smp", *((g_address*) (mappedLower+G_SMP_STARTUP_AREA_AP_ENTRY)));
	logDebug("%! initial AP counter value: %i", "smp", *((uint32_t*) (mappedLower+G_SMP_STARTUP_AREA_AP_COUNTER)));

	// Create enough stacks for all APs
	auto stackArray = (g_physical_address*) (mappedLower + G_SMP_STARTUP_AREA_AP_STACK_ARRAY);
	for(uint32_t i = 0; i < processorGetNumberOfProcessors() - 1; i++)
	{
		g_physical_address stackPhysical = memoryPhysicalAllocate();
		if(stackPhysical == 0)
		{
			logInfo("%*%! could not allocate physical page for AP stack", 0x0C, "smp");
			return;
		}

		g_virtual_address stackVirtual = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		if(stackVirtual == 0)
		{
			logInfo("%*%! could not allocate virtual range for AP stack", 0x0C, "smp");
			return;
		}

		pagingMapPage(stackVirtual, stackPhysical, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);

		g_virtual_address stackTop = (stackVirtual + G_PAGE_SIZE);
		stackArray[i] = stackTop;

		logDebug("%! created AP stack (%h -> %h) placed at %h", "smp", stackArray[i], stackPhysical,
		        ((g_address)&stackArray[i]) - mappedLower);
	}

	// Copy start object from ramdisk to lower memory
	const char* apStartupPath = "system/lib/apstartup.o";
	g_ramdisk_entry* startupObject = ramdiskFindAbsolute(apStartupPath);
	if(startupObject == nullptr)
	{
		logInfo("%*%! could not initialize due to missing apstartup object at '%s'", 0x0C, "smp", apStartupPath);
		return;
	}
	memoryCopy((uint8_t*) (mappedLower + G_SMP_STARTUP_AREA_CODE_START), startupObject->data, startupObject->dataSize);

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

	logDebug("%! initial AP counter value: %i", "smp", *((uint32_t*) (mappedLower+G_SMP_STARTUP_AREA_AP_COUNTER)));
}

void smpInitializeCore(g_processor* cpu)
{
	// Calculate the vector value for the code start
	uint32_t vectorValue = (G_SMP_STARTUP_AREA_CODE_START >> 12) & 0xFF;

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
