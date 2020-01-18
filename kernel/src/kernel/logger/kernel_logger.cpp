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

#include "kernel/logger/kernel_logger.hpp"

#include "shared/logger/logger.hpp"
#include "shared/video/console_video.hpp"
#include "shared/system/serial_port.hpp"
#include "shared/system/bios_data_area.hpp"
#include "shared/debug/debug_interface.hpp"

void kernelLoggerInitialize(g_setup_information* info)
{
	kernelLoggerInitializeComPorts();
	kernelLoggerPrintHeader(info);
}

void kernelLoggerPrintHeader(g_setup_information* info)
{
	if(!G_PRETTY_BOOT)
		consoleVideoClear();

	logInfon("");
	logInfon("");
	logInfon("");
	consoleVideoSetColor(0x90);
	logInfon("Ghost Kernel");
	consoleVideoSetColor(0x0F);
	logInfo(" Version %i.%i.%i", G_VERSION_MAJOR, G_VERSION_MINOR, G_VERSION_PATCH);
	logInfo("");
	logInfo("  Copyright (C) 2020, Max Schluessel <lokoxe@gmail.com>");
	logInfo("");
	logInfo("%! initializing core services", "kernel");

	logDebug("%! setup information:", "kernel");
	logDebug("%#   reserved: %h - %h", info->kernelImageStart, info->kernelImageEnd);
	logDebug("%#   stack:    %h - %h", info->stackStart, info->stackEnd);
	logDebug("%#   bitmap:   %h - %h", info->bitmapStart, info->bitmapEnd);
	logDebug("%#   heap:     %h - %h", info->heapStart, info->heapEnd);
	logDebug("%#   mbstruct: %h", info->multibootInformation);
	logDebug("%! started", "kern");
	logDebug("%! got setup information at %h", "kern", info);
}

void kernelLoggerInitializeComPorts()
{
	g_com_port_information comPortInfo = biosDataArea->comPortInfo;
	if(comPortInfo.com1 == 0)
	{
		logInfo("%! COM1 port not available for serial debug output", "logger");
		return;
	}

	serialPortInitialize(comPortInfo.com1, false);
	loggerEnableSerial(true);
	debugInterfaceInitialize(comPortInfo.com1);
}
