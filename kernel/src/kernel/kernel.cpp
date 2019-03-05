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
#include "shared/system/mutex.hpp"
#include "kernel/system/system.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/logger/kernel_logger.hpp"

#include "shared/runtime/constructors.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"
#include "shared/system/serial_port.hpp"

#include "kernel/calls/syscall.hpp"
#include "kernel/tasking/elf32_loader.hpp"

static g_mutex bootstrapCoreLock;
static g_mutex applicationCoreLock;

/* TESTING */
int as[4] =
{ 0, 0, 0, 0 };
int bs[4] =
{ 0, 0, 0, 0 };

g_mutex testMutex;

#include "kernel/tasking/wait.hpp"
void test()
{
	mutexAcquire(&testMutex);
	logInfo("#%i: Task %i (Priv %i) says hello from test()!", processorGetCurrentId(), taskingGetLocal()->current->id,
			taskingGetLocal()->current->securityLevel);
	mutexRelease(&testMutex);
	int x = 0;
	for(;;)
	{
		as[processorGetCurrentId()]++;

		if(as[processorGetCurrentId()] % 10000 == 0)
			logInfo("#%i: A(%i %i %i %i), B(%i %i %i %i)", processorGetCurrentId(), as[0], as[1], as[2], as[3], bs[0], bs[1], bs[2], bs[3]);

		taskingKernelThreadYield();
	}

	taskingKernelThreadExit();
}

void test2()
{
	mutexAcquire(&testMutex);
	logInfo("#%i: Task %i (Priv %i) says hello from test2()!", processorGetCurrentId(), taskingGetLocal()->current->id,
			taskingGetLocal()->current->securityLevel);
	mutexRelease(&testMutex);
	for(;;)
	{
		bs[processorGetCurrentId()]++;
		taskingKernelThreadYield();
	}

	taskingKernelThreadExit();
}

/* TESTING */

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
	logDebug("%! has entered kernel", "bsp");
	mutexAcquire(&bootstrapCoreLock, false);
	systemInitializeBsp(initialPdPhys);
	taskingInitializeBsp();
	syscallRegisterAll();

	// TEST THREADS
	g_process* testProc = taskingCreateProcess();
	taskingAssign(taskingGetLocal(), taskingCreateThread((g_virtual_address) test, testProc, G_SECURITY_LEVEL_KERNEL));
	taskingAssign(taskingGetLocal(), taskingCreateThread((g_virtual_address) test2, testProc, G_SECURITY_LEVEL_KERNEL));

	g_task* userTask;
	elf32SpawnFromRamdisk(ramdiskFindAbsolute("applications/init.bin"), G_SECURITY_LEVEL_APPLICATION, &userTask);

	g_task* userTask2;
	elf32SpawnFromRamdisk(ramdiskFindAbsolute("applications/init.bin"), G_SECURITY_LEVEL_APPLICATION, &userTask2);
	// TEST THREADS END

	mutexRelease(&bootstrapCoreLock, false);
	systemWaitForApplicationCores();
	interruptsEnable();
	for(;;)
		asm("hlt");
}

void kernelRunApplicationCore()
{
	logDebug("%! has entered kernel, waiting for bsp", "ap");
	mutexAcquire(&bootstrapCoreLock, false);
	mutexRelease(&bootstrapCoreLock, false);

	mutexAcquire(&applicationCoreLock, false);

	logDebug("%! initializing %i", "ap", processorGetCurrentId());
	systemInitializeAp();
	taskingInitializeAp();

	// TEST THREADS
	g_process* testProc = taskingCreateProcess();
	taskingAssign(taskingGetLocal(), taskingCreateThread((g_virtual_address) test, testProc, G_SECURITY_LEVEL_KERNEL));
	taskingAssign(taskingGetLocal(), taskingCreateThread((g_virtual_address) test2, testProc, G_SECURITY_LEVEL_KERNEL));

	g_task* userTask;
	elf32SpawnFromRamdisk(ramdiskFindAbsolute("applications/init.bin"), G_SECURITY_LEVEL_APPLICATION, &userTask);

	g_task* userTask2;
	elf32SpawnFromRamdisk(ramdiskFindAbsolute("applications/init.bin"), G_SECURITY_LEVEL_APPLICATION, &userTask2);
	// TEST THREADS END

	mutexRelease(&applicationCoreLock, false);
	systemWaitForApplicationCores();
	interruptsEnable();
	for(;;)
		asm("hlt");
}

void kernelPanic(const char *msg, ...)
{
	logInfo("%*%! an unrecoverable error has occured. reason:", 0x0C, "kernerr");

	loggerManualLock();
	va_list valist;
	va_start(valist, msg);
	loggerPrintFormatted(msg, valist);
	va_end(valist);
	loggerPrintCharacter('\n');
	loggerManualUnlock();

	interruptsDisable();
	for(;;)
		asm("hlt");
}

void kernelHalt()
{
	logInfo("%! execution finished, halting", "postkern");
	interruptsDisable();
	for(;;)
		asm("hlt");
}
