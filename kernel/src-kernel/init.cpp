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

#include <kernel.hpp>
#include <runtime/constructors.hpp>

#include <build_config.hpp>
#include <kernelloader/setup_information.hpp>
#include <logger/logger.hpp>
#include <video/console_video.hpp>
#include "ghost/stdint.h"
#include <system/serial/serial_port.hpp>

/**
 * Does the final loading preparation and starts the kernel.
 *
 * @param setupInformation		the setup information passed by the loader
 */
extern "C" void loadKernel(g_setup_information* setupInformation) {

	g_constructors::call();

	// Call the kernel
	g_kernel::run(setupInformation);

	// Hang after execution
	g_log_info("%! execution finished, halting", "postkern");
	asm("cli");
	for (;;) {
		asm("hlt");
	}
}
