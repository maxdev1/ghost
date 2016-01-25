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

#include "kernel.hpp"
#include "kernelloader/setup_information.hpp"
#include "multiboot/multiboot_util.hpp"

#include "ghost/stdint.h"
#include "logger/logger.hpp"
#include "video/console_video.hpp"

#include "system/system.hpp"
#include "system/smp/global_lock.hpp"
#include "system/serial/serial_port.hpp"
#include "system/bios_data_area.hpp"
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

// pointer to the global ramdisk
g_ramdisk* g_kernel_ramdisk;

// pool of virtual address ranges for the kernel
g_address_range_pool* g_kernel_virt_addr_ranges;

// BSP/AP setup locks & counter
static g_global_lock bsp_setup_lock;
static g_global_lock ap_setup_lock;
static int waiting_aps;

// process spawning lock
static g_global_lock system_process_spawn_lock;

/**
 * 
 */
void g_kernel::pre_setup(g_setup_information* info) {

	// initialize COM port
	g_com_port_information comPortInfo = biosDataArea->comPortInfo;
	if (comPortInfo.com1 > 0) {
		g_serial_port::initializePort(comPortInfo.com1, false); // Initialize in poll mode
		g_logger::enableSerialPortLogging();
		g_debug_interface::initialize(comPortInfo.com1);
	} else {
		g_logger::println("%! COM1 port not available for serial debug output", "logger");
	}

	// print header
	print_header(info);

	// Initialize physical page allocator from bitmap provided by the loader
	g_pp_allocator::initializeFromBitmap(info->bitmapStart, info->bitmapEnd);

	uint32_t mbs = (g_pp_allocator::getFreePageCount() * G_PAGE_SIZE / 1024) / 1024;
	g_log_info("%! memory: %iMB", "kern", mbs);

	// Initialize the kernel heap
	g_kernel_heap::initialize(info->heapStart, info->heapEnd);

	// Find ramdisk module
	g_multiboot_module* rd_module = g_multiboot_util::findModule(info->multibootInformation, "/boot/ramdisk");
	if (rd_module == 0) {
		panic("%! ramdisk does not exist", "kern");
	}

	// Create virtual range pool for kernel ranges
	g_kernel_virt_addr_ranges = new g_address_range_pool();
	g_kernel_virt_addr_ranges->initialize(G_CONST_KERNEL_VIRTUAL_RANGES_START, G_CONST_KERNEL_VIRTUAL_RANGES_END);

	// Remap the ramdisk module into the kernels ranges & load the ramdisk
	load_ramdisk(rd_module);
}

/**
 *
 */
void g_kernel::run(g_setup_information* info) {

	// perform initial setup
	pre_setup(info);

	// copy remaining information from loader information
	g_physical_address initial_pd_physical = info->initialPageDirectoryPhysical;
	g_log_debug("%! unmapping old address space area", "kern");
	for (g_virtual_address i = G_CONST_LOWER_MEMORY_END; i < G_CONST_KERNEL_AREA_START; i += G_PAGE_SIZE) {
		g_address_space::unmap(i);
	}
	// NOTE: pointer to info is now invalid

	// run BSP setup
	run_bsp(initial_pd_physical);
}

/**
 *
 */
void g_kernel::run_bsp(g_physical_address initial_pd_physical) {

	// initialize the temporary paging util
	g_temporary_paging_util::initialize();

	// tell the lower memory allocator which area to use
	g_lower_heap::addArea(G_CONST_LOWER_HEAP_MEMORY_START, G_CONST_LOWER_HEAP_MEMORY_END);

	// perform bsp initialization
	bsp_setup_lock.lock();
	{
		// Initialize processors & interrupt handling
		g_system::initializeBsp(initial_pd_physical);

		// Initialize global descriptor table
		// (AFTER the system, so BSP's id is available)
		g_gdt_manager::prepare();
		g_gdt_manager::initialize();

		// Initialize the scheduler
		g_tasking::initialize();

		// Enable tasking for this core
		g_tasking::enableForThisCore();

		// Initialize filesystem
		g_filesystem::initialize();

		// Create initial process
		load_system_process(G_IDLE_BINARY_NAME, g_thread_priority::IDLE);
		load_system_process(G_INIT_BINARY_NAME, g_thread_priority::NORMAL);
	}
	bsp_setup_lock.unlock();
	/* BSP INITIALIZATION END */

	// wait for APs
	waiting_aps = g_system::getNumberOfProcessors() - 1;
	g_log_info("%! waiting for %i application processors", "kern", waiting_aps);
	while (waiting_aps > 0) {
		asm("pause");
	}

	// Enable interrupts and wait until the first interrupt causes the scheduler to switch to the initial process
	g_log_info("%! leaving initialization", "kern");
	asm("sti");
	for (;;) {
		asm("hlt");
	}
}

/**
 *
 */
void g_kernel::run_ap() {

	ap_setup_lock.lock();
	{
		uint32_t core = g_system::currentProcessorId();
		g_log_debug("%! core %i ready for initialization", "kernap", core);

		// Debug ESP output
		uint32_t esp;
		asm("mov %%esp, %0":"=g"(esp));
		g_log_debug("%! esp is %h", "kernap", esp);

		// Wait for BSP to finish setup
		g_log_debug("%! waiting for bsp to finish setup", "kernap");
		bsp_setup_lock.lock();
		g_log_debug("%! core %i got ready state from bsp", "kernap", core);
		bsp_setup_lock.unlock();

		// Initialize GDT
		g_gdt_manager::initialize();

		// Initialize for AP
		g_system::initializeAp();

		// Enable tasking for this core
		g_tasking::enableForThisCore();

		// Leave initialization
		load_system_process(G_IDLE_BINARY_NAME, g_thread_priority::IDLE);

		// tell bsp that one more is done
		--waiting_aps;
	}
	ap_setup_lock.unlock();

	// wait for APs
	g_log_debug("%! waiting for %i application processors", "kernap", waiting_aps);
	while (waiting_aps > 0) {
		asm("pause");
	}

	// Enable interrupts and wait until the first interrupt causes the scheduler to switch to the initial process
	g_log_debug("%! leaving initialization", "kernap");
	asm("sti");
	for (;;) {
		asm("hlt");
	}
}

/**
 * 
 */
void g_kernel::load_system_process(const char* binary_path, g_thread_priority priority) {

	system_process_spawn_lock.lock();

	g_ramdisk_entry* entry = g_kernel_ramdisk->findAbsolute(binary_path);
	if (entry) {
		g_thread* systemProcess;
		g_elf32_spawn_status status = g_elf32_loader::spawnFromRamdisk(entry, G_SECURITY_LEVEL_KERNEL, &systemProcess, 0, true);

		if (status != ELF32_SPAWN_STATUS_SUCCESSFUL) {
			if (status == ELF32_SPAWN_STATUS_VALIDATION_ERROR) {
				panic("%! \"%s\" is not a valid elf32 binary", "kern", binary_path);

			} else if (status == ELF32_SPAWN_STATUS_PROCESS_CREATION_FAILED) {
				panic("%! \"%s\" could not be loaded, error creating process", "kern", binary_path);

			} else {
				panic("%! \"%s\" could not be loaded", "kern", binary_path);
			}
		}

		systemProcess->priority = priority;
		g_log_info("%! \"%s\" spawned to process %i", "kern", binary_path, systemProcess->id);
	} else {
		panic("%! \"%s\" not found", "kern", binary_path);
	}

	system_process_spawn_lock.unlock();
}
/**
 *
 */
void g_kernel::print_header(g_setup_information* info) {

	// Print header
	g_console_video::clear();
	g_log_infon("");
	g_log_infon("");
	g_log_infon("");
	g_console_video::setColor(0x90);
	g_log_infon("Ghost Kernel");
	g_console_video::setColor(0x0F);
	g_log_info(" Version %i.%i.%i", G_VERSION_MAJOR, G_VERSION_MINOR, G_VERSION_PATCH);
	g_log_info("");
	g_log_info("  Copyright (C) 2016, Max Schluessel <lokoxe@gmail.com>");
	g_log_info("");
	g_log_info("%! loading", "prekern");

	// Print setup information
	g_log_debug("%! setup information:", "prekern");
	g_log_debug("%#   reserved: %h - %h", info->kernelImageStart, info->kernelImageEnd);
	g_log_debug("%#   stack:    %h - %h", info->stackStart, info->stackEnd);
	g_log_debug("%#   bitmap:   %h - %h", info->bitmapStart, info->bitmapEnd);
	g_log_debug("%#   heap:     %h - %h", info->heapStart, info->heapEnd);
	g_log_debug("%#   mbstruct: %h", info->multibootInformation);
	g_log_debug("%! started", "kern");
	g_log_debug("%! got setup information at %h", "kern", info);

}

/**
 *
 */
void g_kernel::load_ramdisk(g_multiboot_module* ramdiskModule) {

	int ramdiskPages = PAGE_ALIGN_UP(ramdiskModule->moduleEnd - ramdiskModule->moduleStart) / G_PAGE_SIZE;

	g_virtual_address ramdiskNewLocation = g_kernel_virt_addr_ranges->allocate(ramdiskPages);
	if (ramdiskNewLocation == 0) {
		panic("%! not enough virtual space for ramdisk remapping", "kern");
	}

	for (int i = 0; i < ramdiskPages; i++) {
		g_virtual_address virt = ramdiskNewLocation + i * G_PAGE_SIZE;
		g_physical_address phys = g_address_space::virtual_to_physical(ramdiskModule->moduleStart + i * G_PAGE_SIZE);
		g_address_space::map(virt, phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	ramdiskModule->moduleEnd = ramdiskNewLocation + (ramdiskModule->moduleEnd - ramdiskModule->moduleStart);
	ramdiskModule->moduleStart = ramdiskNewLocation;

	g_kernel_ramdisk = new g_ramdisk();
	g_kernel_ramdisk->load(ramdiskModule);
	g_log_info("%! ramdisk loaded", "kern");
}

/**
 * 
 */
void __attribute__((no_return)) g_kernel::panic(const char* msg, ...) {

	g_logger::manualLock();

	g_log_info("%*%! an unrecoverable error has occured. reason:", 0x0C, "kernerr");

	va_list valist;
	va_start(valist, msg);
	g_logger::printFormatted(msg, valist);
	va_end(valist);
	g_logger::printCharacter('\n');

	g_logger::manualUnlock();

	asm("cli");
	for (;;) {
		asm("hlt");
	}
}

