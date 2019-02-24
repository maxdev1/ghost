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

#include "logger/main_logger.hpp"
#include "logger/logger.hpp"
#include "video/console_video.hpp"
#include "system/serial/serial_port.hpp"
#include "system/bios_data_area.hpp"
#include "debug/debug_interface.hpp"

void mainLoggerPrintHeader(g_setup_information* info) {

	if (!G_PRETTY_BOOT) {
		g_console_video::clear();
	}
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

	g_log_debug("%! setup information:", "prekern");
	g_log_debug("%#   reserved: %h - %h", info->kernelImageStart, info->kernelImageEnd);
	g_log_debug("%#   stack:    %h - %h", info->stackStart, info->stackEnd);
	g_log_debug("%#   bitmap:   %h - %h", info->bitmapStart, info->bitmapEnd);
	g_log_debug("%#   heap:     %h - %h", info->heapStart, info->heapEnd);
	g_log_debug("%#   mbstruct: %h", info->multibootInformation);
	g_log_debug("%! started", "kern");
	g_log_debug("%! got setup information at %h", "kern", info);
}

void mainLoggerInitializeComPorts() {

	g_com_port_information comPortInfo = biosDataArea->comPortInfo;

	if (comPortInfo.com1 > 0) {
		g_serial_port::initializePort(comPortInfo.com1, false);
		g_logger::enableSerialPortLogging();
		g_debug_interface::initialize(comPortInfo.com1);

	} else {
		g_log_info("%! COM1 port not available for serial debug output", "logger");
	}
}
