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

#include <loader.hpp>
#include <runtime/constructors.hpp>

#include <logger/logger.hpp>
#include <video/console_video.hpp>
#include <system/serial/serial_port.hpp>

/**
 * Initialization function, called from the loader assembly. Checks the
 * multiboot magic number and then passes the multiboot structure to the
 * loader for further initialization.
 *
 * @param multibootStruct	the multiboot structure passed by GRUB
 * @param magicNumber		the magic number passed by GRUB
 */
extern "C" void initializeLoader(g_multiboot_information* multibootStruct, uint32_t magicNumber) {

	g_constructors::call();

	// Initialize COM port logging
	g_logger::initializeSerial();

	// Clear the console and print the header colored
	g_console_video::clear();
	g_log_info("");
	g_console_video::setColor(0x90);
	g_log_infon("Ghost Loader");
	g_console_video::setColor(0x0F);
	g_log_infon(" Version 1.1");
	g_log_info("");

	g_log_info("%! checking magic number", "early");

	if (magicNumber == G_MULTIBOOT_BOOTLOADER_MAGIC) {
		g_log_info("%! initializing loader", "early");
		g_loader::initialize(multibootStruct);

	} else {
		g_log_info("%! invalid magic number in multiboot struct", "early");
	}
}
