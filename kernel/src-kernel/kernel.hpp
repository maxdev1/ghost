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

#ifndef __KERNEL__
#define __KERNEL__

#include "build_config.hpp"
#include "kernelloader/setup_information.hpp"
#include "memory/collections/address_range_pool.hpp"
#include "ramdisk/ramdisk.hpp"
#include "tasking/thread.hpp"

/**
 * Global ramdisk instance.
 */
extern g_ramdisk* kernelRamdisk;

/**
 * Virtual range pool that allocates addresses within the kernels virtual memory area.
 */
extern g_address_range_pool* kernelMemoryVirtualRangePool;

/**
 * Called before any other initialization happens. The information provided by
 * the loader is interpreted and the essential kernel modules are initialized.
 *
 * @param info setup information structure
 */
void kernelPerformInitialSetup(g_setup_information* info);

/**
 * Initializes the basic memory functionality of the kernel.
 *
 * @param info setup information structure
 */
void kernelInitializeMemory(g_setup_information* info);

/**
 * Initializes the kernel ramdisk instance.
 *
 * @param info multiboot information structure
 */
void kernelInitializeRamdisk(g_multiboot_information* info);

/**
 * Unmaps the memory that was provided by the kernel loader for initial setup.
 */
void kernelUnmapSetupMemory();

/**
 * Halts the system and prints a diagnostic message.
 *
 * @param message format for the panic message
 * @param ... variable argument list
 */
void kernelPanic(const char* message, ...);

/**
 * Does the initial kernel setup.
 *
 * @param info setup information structure
 */
void kernelRun(g_setup_information* info);

/**
 * Performs initialization of the bootstrap processor (BSP).
 *
 * @param initial_pd_physical
 */
void kernelRunBootstrapCore(g_physical_address initialPageDirectory);

/**
 * Performs initialization of an application processor. This function is
 * executed on every single application processor (AP).
 */
void kernelRunApplicationCore();

/**
 * Loads a binary as an initial system task.
 *
 * @param binaryPath path to the binary
 * @param priority thread priority
 */
void kernelLoadSystemProcess(const char* binaryPath, g_thread_priority priority);

/**
 * Used to enable interrupts once the basic kernel setup has finished.
 * Once the first timer interrupt occurs, the scheduler automatically switches
 * to the first suitable task.
 */
void kernelEnableInterrupts();

/**
 * Disables interrupts. This causes the system to halt as no tasks will be scheduled
 * anymore. Usually is only called in case of kernel panic.
 */
void kernelDisableInterrupts();

/**
 * Sleeps the current execution thread until the number of waiting application
 * cores is zero.
 */
void kernelWaitForApplicationCores();

#endif
