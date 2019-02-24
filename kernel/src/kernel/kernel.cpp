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

#include "kernel/kernel.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/system/mutex.hpp"
#include "kernel/system/system.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/logger/kernel_logger.hpp"

#include "shared/runtime/constructors.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"
#include "shared/system/serial_port.hpp"

static g_mutex bootstrapCoreLock;
static g_mutex applicationCoreLock;

extern "C" void kernelMain(g_setup_information* setupInformation)
{
	runtimeAbiCallGlobalConstructors();

	if(G_PRETTY_BOOT)
		prettyBootEnable(false);
	else
		consoleVideoClear();

	kernelInitialize(setupInformation);
	g_address initialPdPhys = setupInformation->initialPageDirectoryPhysical;
	memoryUnmapSetupMemory();
	kernelRunBootstrapCore(initialPdPhys);
	kernelHalt();
}

void kernelInitialize(g_setup_information* setupInformation)
{
	kernelLoggerInitialize(setupInformation);
	memoryInitialize(setupInformation);

	g_multiboot_module* ramdiskModule = multibootFindModule(setupInformation->multibootInformation, "/boot/ramdisk");
	if(!ramdiskModule)
	{
		G_PRETTY_BOOT_FAIL("Ramdisk not found (did you supply enough memory?");
		kernelPanic("%! ramdisk not found (did you supply enough memory?)", "kern");
	}
	ramdiskLoadFromModule(ramdiskModule);
}

void kernelRunBootstrapCore(g_physical_address initialPdPhys)
{
	mutexAcquire(&bootstrapCoreLock);
	systemInitializeBsp(initialPdPhys);
	taskingInitializeBsp();
#warning "TODO: initialize filesystem here"
#warning "TODO: load idle process"
	mutexRelease(&bootstrapCoreLock);

	systemWaitForApplicationCores();
	interruptsEnable();
}

void kernelRunApplicationCore()
{
	mutexAcquire(&bootstrapCoreLock);
	mutexRelease(&bootstrapCoreLock);

	mutexAcquire(&applicationCoreLock);
	systemInitializeAp();
	taskingInitializeAp();
#warning "TODO load idle process"
	mutexRelease(&applicationCoreLock);

	systemWaitForApplicationCores();
	interruptsEnable();
}

void kernelPanic(const char *msg, ...)
{
	loggerManualLock();
	logInfo("%*%! an unrecoverable error has occured. reason:", 0x0C, "kernerr");

	va_list valist;
	va_start(valist, msg);
	loggerPrintFormatted(msg, valist);
	va_end(valist);
	loggerPrintCharacter('\n');

	interruptsDisable();
}

void kernelHalt()
{
	logInfo("%! execution finished, halting", "postkern");
	interruptsDisable();
}
