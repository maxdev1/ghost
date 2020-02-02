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
#include "kernel/calls/syscall.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/ipc/message.hpp"

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
	loggerInitialize();
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
	mutexInitialize(&bootstrapCoreLock);
	mutexAcquire(&bootstrapCoreLock, false);

	systemInitializeBsp(initialPdPhys);
	filesystemInitialize();
	pipeInitialize();
	messageInitialize();

	taskingInitializeBsp();
	syscallRegisterAll();

	g_process* initializationProcess = taskingCreateProcess();
	taskingAssign(taskingGetLocal(), taskingCreateThread((g_virtual_address) kernelInitializationThread, initializationProcess, G_SECURITY_LEVEL_KERNEL));

	logInfo("%! booting on %i cores", "kernel", processorGetNumberOfProcessors());
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

	mutexInitialize(&applicationCoreLock);
	mutexAcquire(&applicationCoreLock, false);

	logDebug("%! initializing %i", "ap", processorGetCurrentId());
	systemInitializeAp();
	taskingInitializeAp();

	mutexRelease(&applicationCoreLock, false);
	systemWaitForApplicationCores();
	interruptsEnable();
	for(;;)
		asm("hlt");
}

void testSpawn(const char* path)
{
	g_task* currentTask = taskingGetCurrentTask();
	g_fd fd;
	g_fs_open_status open = filesystemOpen(path, G_FILE_FLAG_MODE_READ, currentTask, &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		g_process* outProcess;
		g_spawn_status spawn = taskingSpawn(currentTask, fd, G_SECURITY_LEVEL_DRIVER, &outProcess);
		if(spawn == G_SPAWN_STATUS_SUCCESSFUL)
			logInfo("%! %s -> %i", "test", path, outProcess->id);
		else
			logInfo("%! failed to spawn %s with status %i", "kernel", path, spawn);
	} else
		logInfo("%! failed to find %s with status %i", "kernel", path, open);
}

void kernelInitializationThread()
{
	g_task* currentTask = taskingGetCurrentTask();
	logInfo("%! initializing system services in task %i", "kernel", currentTask->id);

	testSpawn("/applications/ps2driver.bin");
	testSpawn("/applications/vbedriver.bin");
	testSpawn("/applications/windowserver.bin");

	taskingKernelThreadExit();
}

void kernelPanic(const char *msg, ...)
{
	interruptsDisable();
	logInfo("%*%! unrecoverable error on processor %i", 0x0C, "kernerr", processorGetCurrentId());

	loggerManualLock();
	va_list valist;
	va_start(valist, msg);
	loggerPrintFormatted(msg, valist);
	va_end(valist);
	loggerPrintCharacter('\n');
	loggerManualUnlock();

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
