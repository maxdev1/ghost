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
#include "loader/memory/physical.hpp"
#include "loader/setup_information.hpp"
#include "shared/debug/debug_interface.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"
#include "shared/system/bios_data_area.hpp"
#include "shared/system/serial_port.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"

// Symbol provided by linker at end of loader binary
extern "C" void* endAddress;

extern "C" void loaderMain(g_multiboot_information* multiboot, uint32_t magicNumber)
{
	if(magicNumber != G_MULTIBOOT_BOOTLOADER_MAGIC)
		panic("%! invalid magic number in multiboot struct", "mboot");

	if(multiboot->flags & G_MULTIBOOT_FLAGS_BOOTLDNAM)
		logInfo("%! loaded by: %s", "mboot", multiboot->bootloaderName);

	loaderEnableLoggingFeatures();
	setupInformation.multibootInformation = multiboot;
	loaderInitializeMemory();
	loaderStartKernel();
}

g_physical_address loaderSetupGdt()
{
	g_address loaderEnd = G_PAGE_ALIGN_UP((g_address) &endAddress);
	g_address gdtPage = memoryPhysicalAllocateInitial(loaderEnd, 1);
	gdtInitialize(gdtPage);
	return gdtPage + G_PAGE_SIZE;
}

/**
 * The bootloader has loaded the kernel and the ramdisk as multiboot modules
 * into memory somewhere after 0x00100000. The GDT is allocated in the first
 * found free physical page. The physical memory map is interpreted to
 * determine how much space is required for the bitmap array used by the
 * physical allocator. The bitmap array starts right after the GDT page.
 *
 * The end of the bitmap array is also the end of the "reserved area", which
 * is identity-mapped so that the kernel can access this data.
 *
 * Since there is often some free space before the multiboot modules,
 * the memory layout usually looks like this after initializing:
 *
 * [0x0-0x100000]...[gdt][mb-kernel][mb-ramdisk][bitmap-array]...
 */
void loaderInitializeMemory()
{
	g_physical_address gdtEnd = loaderSetupGdt();

	uint32_t bitmapRequiredMemory = memoryPhysicalReadMemoryMap(gdtEnd, 0);
	setupInformation.bitmapArrayStart = memoryPhysicalAllocateInitial(gdtEnd, G_PAGE_ALIGN_UP(bitmapRequiredMemory) / G_PAGE_SIZE);
	setupInformation.bitmapArrayEnd = G_PAGE_ALIGN_UP(setupInformation.bitmapArrayStart + bitmapRequiredMemory);
	memoryPhysicalReadMemoryMap(setupInformation.bitmapArrayEnd, setupInformation.bitmapArrayStart);
	bitmapPageAllocatorInitialize(&memoryPhysicalAllocator, (g_bitmap_header*) setupInformation.bitmapArrayStart);

	setupInformation.initialPageDirectoryPhysical = pagingInitialize(setupInformation.bitmapArrayEnd);
}

void loaderStartKernel()
{
	g_multiboot_module* kernelModule = multibootFindModule(setupInformation.multibootInformation, "/boot/kernel");
	if(!kernelModule)
	{
		G_PRETTY_BOOT_FAIL("Kernel module not found");
		panic("%! kernel module not found", "loader");
	}

	G_PRETTY_BOOT_STATUS_P(5);
	logInfo("%! found kernel binary at %h, loading...", "loader", kernelModule->moduleStart);
	logInfo("");

	kernelLoaderLoad(kernelModule);
}

void loaderEnableLoggingFeatures()
{
	g_com_port_information comPortInfo = biosDataArea->comPortInfo;
	if(comPortInfo.com1 > 0)
	{
		serialPortInitialize(comPortInfo.com1, false); // Initialize in poll mode
		loggerEnableSerial(true);
		debugInterfaceInitialize(comPortInfo.com1);
	}
	else
	{
		logWarn("%! COM1 port not available for serial debug output", "logger");
	}

	if(G_PRETTY_BOOT)
	{
		prettyBootEnable();
	}
	else
	{
		consoleVideoClear();
	}

	logInfo("");
	consoleVideoSetColor(0x90);
	logInfon("Ghost Loader");
	consoleVideoSetColor(0x0F);
	logInfo(" Version %i.%i.%i", G_LOADER_VERSION_MAJOR, G_LOADER_VERSION_MINOR, G_LOADER_VERSION_PATCH);
	logInfo("");
	G_PRETTY_BOOT_STATUS_P(1);
}
