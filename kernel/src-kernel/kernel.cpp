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

#include "ghost/stdint.h"

#include "kernel.hpp"
#include "kernelloader/setup_information.hpp"
#include "multiboot/multiboot_util.hpp"
#include "logger/main_logger.hpp"
#include "logger/logger.hpp"

#include "system/system.hpp"
#include "system/pci/pci.hpp"
#include "system/smp/mutex.hpp"
#include "tasking/tasking.hpp"
#include "filesystem/filesystem.hpp"

#include "memory/gdt/gdt_manager.hpp"
#include "memory/kernel_heap.hpp"
#include "memory/physical/pp_allocator.hpp"
#include "memory/paging.hpp"
#include "memory/address_space.hpp"
#include "memory/temporary_paging_util.hpp"
#include "memory/lower_heap.hpp"
#include "memory/constants.hpp"
#include "memory/collections/address_stack.hpp"

#include "executable/elf32_loader.hpp"
#include "memory/collections/address_range_pool.hpp"
#include "video/pretty_boot.hpp"

g_ramdisk *kernelRamdisk;
g_address_range_pool *kernelMemoryVirtualRangePool;

static mutex_t bootstrapCoreLock;
static mutex_t applicationCoreLock;
static mutex_t systemProcessSpawnLock;
static int applicationCoresWaiting;

void kernelRun(g_setup_information *info)
{
	uint32_t initialPdPhys = info->initialPageDirectoryPhysical;
	kernelPerformInitialSetup(info);
	kernelUnmapSetupMemory();
	// "info" is now invalid

	kernelRunBootstrapCore(initialPdPhys);
}

void kernelPerformInitialSetup(g_setup_information *info)
{
	mainLoggerInitializeComPorts();
	mainLoggerPrintHeader(info);

	kernelInitializeMemory(info);
	kernelInitializeRamdisk(info->multibootInformation);
}

void kernelRunBootstrapCore(g_physical_address initialPageDirectoryPhysical)
{
	g_temporary_paging_util::initialize();

	g_lower_heap::addArea(G_CONST_LOWER_HEAP_MEMORY_START, G_CONST_LOWER_HEAP_MEMORY_END);

	mutexAcquire(&bootstrapCoreLock);

	g_system::initializeBsp(initialPageDirectoryPhysical);

	g_gdt_manager::prepare();
	g_gdt_manager::initialize();

	g_tasking::initialize();
	g_tasking::enableForThisCore();

	G_PRETTY_BOOT_STATUS("Initializing filesystem", 80);
	g_filesystem::initialize();

	g_pci::initialize();

	G_PRETTY_BOOT_STATUS("Loading system binaries", 90);
	kernelLoadSystemProcess(G_IDLE_BINARY_NAME, G_THREAD_PRIORITY_IDLE);
	kernelLoadSystemProcess(G_INIT_BINARY_NAME, G_THREAD_PRIORITY_NORMAL);
	kernelLoadSystemProcess(G_INIT_BINARY_NAME, G_THREAD_PRIORITY_NORMAL);
	G_PRETTY_BOOT_STATUS("Starting user space", 100);

	mutexRelease(&bootstrapCoreLock);

	applicationCoresWaiting = g_system::getNumberOfProcessors() - 1;
	kernelWaitForApplicationCores();
	kernelEnableInterrupts();
}

void kernelRunApplicationCore()
{
	mutexAcquire(&applicationCoreLock);

	g_log_debug("%! waiting for bsp to finish setup", "kernap");
	mutexAcquire(&bootstrapCoreLock);
	g_log_debug("%! core %i got ready state from bsp", "kernap", g_system::currentProcessorId());
	mutexRelease(&bootstrapCoreLock);

	g_gdt_manager::initialize();
	g_system::initializeAp();
	g_tasking::enableForThisCore();
	kernelLoadSystemProcess(G_IDLE_BINARY_NAME, G_THREAD_PRIORITY_IDLE);

	--applicationCoresWaiting;

	mutexRelease(&applicationCoreLock);

	kernelWaitForApplicationCores();
	kernelEnableInterrupts();
}

void kernelInitializeMemory(g_setup_information *info)
{
	G_PRETTY_BOOT_STATUS("Checking available memory", 20);

	g_pp_allocator::initializeFromBitmap(info->bitmapStart, info->bitmapEnd);
	uint32_t size = (g_pp_allocator::getFreePageCount() * G_PAGE_SIZE / 1024) / 1024;
	g_log_info("%! available memory: %iMB", "kern", size);

	g_kernel_heap::initialize(info->heapStart, info->heapEnd);

	kernelMemoryVirtualRangePool = new g_address_range_pool();
	kernelMemoryVirtualRangePool->initialize(G_CONST_KERNEL_VIRTUAL_RANGES_START, G_CONST_KERNEL_VIRTUAL_RANGES_END);
}

void kernelInitializeRamdisk(g_multiboot_information *info)
{
	G_PRETTY_BOOT_STATUS("Loading ramdisk", 35);

	g_multiboot_module *rd_module = g_multiboot_util::findModule(info, "/boot/ramdisk");
	if (rd_module == 0)
	{
		G_PRETTY_BOOT_FAIL("Ramdisk not found (did you supply enough memory?");
		kernelPanic("%! ramdisk not found (did you supply enough memory?)", "kern");
	}

	kernelRamdisk = ramdiskLoadFromModule(rd_module);
}

void kernelUnmapSetupMemory()
{
	for (g_virtual_address i = G_CONST_LOWER_MEMORY_END; i < G_CONST_KERNEL_AREA_START; i += G_PAGE_SIZE)
	{
		g_address_space::unmap(i);
	}
}

void kernelLoadSystemProcess(const char *binary_path, g_thread_priority priority)
{
	mutexAcquire(&systemProcessSpawnLock);

	g_ramdisk_entry *entry = kernelRamdisk->findAbsolute(binary_path);
	if (entry)
	{
		g_thread *systemProcess;
		g_elf32_spawn_status status = g_elf32_loader::spawnFromRamdisk(entry, G_SECURITY_LEVEL_KERNEL, &systemProcess, true, priority);

		if (status != ELF32_SPAWN_STATUS_SUCCESSFUL)
		{
			if (status == ELF32_SPAWN_STATUS_VALIDATION_ERROR)
			{
				kernelPanic("%! \"%s\" is not a valid elf32 binary", "kern", binary_path);
			}
			else if (status == ELF32_SPAWN_STATUS_PROCESS_CREATION_FAILED)
			{
				kernelPanic("%! \"%s\" could not be loaded, error creating process", "kern", binary_path);
			}
			else
			{
				kernelPanic("%! \"%s\" could not be loaded", "kern", binary_path);
			}
		}

		g_log_info("%! \"%s\" spawned to process %i", "kern", binary_path, systemProcess->id);
	}
	else
	{
		kernelPanic("%! \"%s\" not found", "kern", binary_path);
	}

	mutexRelease(&systemProcessSpawnLock);
}

void kernelEnableInterrupts()
{
	asm("sti");
	for (;;)
		asm("hlt");
}

void kernelDisableInterrupts()
{
	asm("cli");
	for (;;)
		asm("hlt");
}

void kernelWaitForApplicationCores()
{
	g_log_info("%! waiting for %i application processors", "kern", applicationCoresWaiting);
	while (applicationCoresWaiting > 0)
	{
		asm("pause");
	}
}

void kernelPanic(const char *msg, ...)
{
	g_logger::manualLock();

	g_log_info("%*%! an unrecoverable error has occured. reason:", 0x0C, "kernerr");

	va_list valist;
	va_start(valist, msg);
	g_logger::printFormatted(msg, valist);
	va_end(valist);
	g_logger::printCharacter('\n');

	g_logger::manualUnlock();

	kernelDisableInterrupts();
}
