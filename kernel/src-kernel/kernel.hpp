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

#ifndef GHOST_KERNEL
#define GHOST_KERNEL

#include <build_config.hpp>
#include <kernelloader/setup_information.hpp>
#include <memory/collections/address_range_pool.hpp>
#include <ramdisk/ramdisk.hpp>
#include <tasking/thread.hpp>

/**
 *
 */
class g_kernel {
public:
	/**
	 * Pool of virtual ranges used in the kernel to map memory.
	 */
	static g_address_range_pool* virtual_range_pool;

	/**
	 * Pointer to the global ramdisk instance.
	 */
	static g_ramdisk* ramdisk;

	/**
	 * Does the initial setup of the kernel. The setup information struct is provided
	 * from the bootloader and contains important information like the position of the
	 * kernel image, or the memory bitmap.
	 *
	 * @param info the setup information provided by the kernel loader
	 */
	static void run(g_setup_information* info);

	/**
	 * BSP setup routine
	 */
	static void run_bsp(g_physical_address initial_pd_physical);

	/**
	 * AP setup routine
	 */
	static void run_ap();

	/**
	 * Loads a system process binary as a process with the specified priority.
	 *
	 * @param path		the ramdisk path to the binary
	 * @param priority	the thread priority to assign
	 */
	static void load_system_process(const char* path, g_thread_priority priority);

	/**
	 * Triggers a kernel panic, means halting the entire system and displaying the given
	 * message/diagnostic information.
	 *
	 * @param message	the panic message, written in a format that the g_logger class understands
	 * @param ...		variable arguments for the message
	 */
	static void panic(const char* message, ...);

private:
	/**
	 *
	 */
	static void print_header(g_setup_information* info);

	/**
	 *
	 */
	static void load_ramdisk(g_multiboot_module* ramdiskModule);

	/**
	 * Performs the very first kernel setup, copying all necessary information that
	 * is provided by the loader.
	 */
	static void pre_setup(g_setup_information* info);

};

#endif
