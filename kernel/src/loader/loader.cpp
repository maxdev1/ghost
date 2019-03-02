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

#include "loader/loader.hpp"
#include "loader/kernel_loader.hpp"

#include "shared/runtime/constructors.hpp"
#include "shared/system/serial_port.hpp"
#include "shared/system/bios_data_area.hpp"

#include "shared/logger/logger.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"
#include "shared/debug/debug_interface.hpp"

// Symbol provided by linker at end of loader binary
extern "C" void* endAddress;

g_setup_information loaderSetupInformation;
g_bitmap_page_allocator loaderPhysicalAllocator;

extern "C" void loaderMain(g_multiboot_information* multibootInformation, uint32_t magicNumber)
{
	if(magicNumber != G_MULTIBOOT_BOOTLOADER_MAGIC)
		loaderPanic("%! invalid magic number in multiboot struct", "early");

	runtimeAbiCallGlobalConstructors();
	loaderEnableLoggingFeatures();
	loaderSetupInformation.multibootInformation = multibootInformation;
	loaderInitializeMemory();
	loaderStartKernel();
}

void loaderInitializeMemory()
{
	g_address gdtEnd = loaderSetupGdt();
	loaderSetupInformation.bitmapStart = loaderFindNextFreePages(gdtEnd, G_PAGE_ALIGN_UP(G_BITMAP_SIZE) / G_PAGE_SIZE);
	loaderSetupInformation.bitmapEnd = G_PAGE_ALIGN_UP(loaderSetupInformation.bitmapStart + G_BITMAP_SIZE);

	g_address reservedAreaEnd = loaderSetupInformation.bitmapEnd;
	bitmapPageAllocatorInitialize(&loaderPhysicalAllocator, (g_bitmap_entry*) loaderSetupInformation.bitmapStart);
	multibootReadMemoryMap(reservedAreaEnd);

	pagingInitialize(reservedAreaEnd);
}

void loaderStartKernel()
{
	logInfo("%! locating kernel binary...", "loader");
	g_multiboot_module* kernelModule = multibootFindModule(loaderSetupInformation.multibootInformation, "/boot/kernel");
	if(!kernelModule)
	{
		G_PRETTY_BOOT_FAIL("Kernel module not found");
		loaderPanic("%! kernel module not found", "loader");
	}

	G_PRETTY_BOOT_STATUS("Loading kernel", 5);
	logInfo("%! found kernel binary at %h, loading...", "loader", kernelModule->moduleStart);

	kernelLoaderLoad(kernelModule);

	loaderPanic("%! something went wrong during boot process, halting", "loader");
}

g_address loaderSetupGdt()
{
	g_address loaderEndAddress = G_PAGE_ALIGN_UP((uint32_t ) &endAddress);
	g_address gdtPage = loaderFindNextFreePages(loaderEndAddress, 1);
	gdtInitialize(gdtPage);
	return gdtPage + G_PAGE_SIZE;
}

g_address loaderFindNextFreePages(g_address start, int count)
{
	logInfo("%! searching for %i free pages (starting at %h)", "loader", count, start);
	g_physical_address location = start;

	while(location < 0xFFFFFFFF)
	{
		bool inModuleRange = false;

		// For each of the required pages, check if it is within a module
		for(int i = 0; i < count; i++)
		{
			g_address pos = location + i * G_PAGE_SIZE;

			// Check one of the modules contains this position
			for(int i = 0; i < (int) loaderSetupInformation.multibootInformation->modulesCount; i++)
			{
				g_multiboot_module* module =
						(g_multiboot_module*) (loaderSetupInformation.multibootInformation->modulesAddress + sizeof(g_multiboot_module) * i);
				g_address moduleStart = G_PAGE_ALIGN_DOWN(module->moduleStart);
				g_address moduleEnd = G_PAGE_ALIGN_UP(module->moduleEnd);

				if(pos >= moduleStart && pos < moduleEnd)
				{
					inModuleRange = true;
					location = moduleEnd;
					break;
				}
			}
		}

		if(!inModuleRange)
		{
			logInfo("%# found: %h", location);
			return location;
		}

		location += G_PAGE_SIZE;
	}

	loaderPanic("%! could not find free memory chunk", "loader");
	return 0;
}

void loaderPanic(const char* msg, ...)
{
	asm("cli");
	logInfo("%! an unrecoverable error has occured. reason:", "lpanic");

	va_list valist;
	va_start(valist, msg);
	loggerPrintFormatted(msg, valist);
	va_end(valist);
	loggerPrintCharacter('\n');

	for(;;)
		asm("hlt");
}

void loaderEnableLoggingFeatures()
{
	g_com_port_information comPortInfo = biosDataArea->comPortInfo;
	if(comPortInfo.com1 > 0)
	{
		serialPortInitialize(comPortInfo.com1, false); // Initialize in poll mode
		loggerEnableSerial(true);
		debugInterfaceInitialize(comPortInfo.com1);
	} else
	{
		logWarn("%! COM1 port not available for serial debug output", "logger");
	}

	if(G_PRETTY_BOOT)
	{
		prettyBootEnable();
	} else
	{
		consoleVideoClear();
	}

	logInfo("");
	consoleVideoSetColor(0x90);
	logInfon("Ghost Loader");
	consoleVideoSetColor(0x0F);
	logInfon(" Version 1.1");
	logInfo("");
	logInfo("%! checking magic number", "early");G_PRETTY_BOOT_STATUS("Validating boot", 1);
}
