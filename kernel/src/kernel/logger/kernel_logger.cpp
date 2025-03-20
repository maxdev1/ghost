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

void kernelLoggerInitialize()
{
	kernelLoggerInitializeComPorts();
	kernelLoggerPrintHeader();
}

void kernelLoggerPrintHeader()
{
	if(!G_PRETTY_BOOT)
		consoleVideoClear();

	logInfon("");
	logInfon("");
	logInfon("");
	consoleVideoSetColor(0xFF4444FF);
	logInfon("Ghost Kernel");
	consoleVideoSetColor(0xFFFFFFFF);
	logInfo(" Version %i.%i.%i", G_VERSION_MAJOR, G_VERSION_MINOR, G_VERSION_PATCH);
	logInfo("  Copyright (C) 2022, Max Schl\x81ssel <lokoxe@gmail.com>");
	logInfo("");
	logInfo("%! initializing core services", "kernel");

	logDebug("%! setup information:", "kernel");
	logDebug("%#   reserved: %h - %h", info->kernelImageStart, info->kernelImageEnd);
	logDebug("%#   stack:    %h - %h", info->stackStart, info->stackEnd);
	logDebug("%#   bitmap:   %h - %h", info->bitmapArrayStart, info->bitmapArrayEnd);
	logDebug("%#   heap:     %h - %h", info->heapStart, info->heapEnd);
	logDebug("%#   mbstruct: %h", info->multibootInformation);
	logDebug("%! started", "kern");
	logDebug("%! got setup information at %h", "kern", info);
}

void kernelLoggerInitializeComPorts()
{
	if(serialPortIsAvailable(G_SERIAL_DEFAULT_COM1))
	{
		serialPortInitialize(G_SERIAL_DEFAULT_COM1, false); // Initialize in poll mode
		loggerEnableSerial(true);
		debugInterfaceInitialize(G_SERIAL_DEFAULT_COM1);
	}
}
