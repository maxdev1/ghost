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
#include "kernel/calls/syscall.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/ipc/message.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/logger/kernel_logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/atoms.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"
#include "shared/setup_information.hpp"
#include "shared/system/mutex.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"

static g_mutex bootstrapCoreLock;
static g_mutex applicationCoreLock;

extern "C" void kernelMain(g_setup_information* setupInformation)
{
	if(G_PRETTY_BOOT)
		prettyBootEnable(false);
	else
		consoleVideoClear();

	kernelLoggerInitialize(setupInformation);

	memoryInitialize(setupInformation);

	g_multiboot_module* ramdiskModule = multibootFindModule(setupInformation->multibootInformation, "/boot/ramdisk");
	if(!ramdiskModule)
	{
		G_PRETTY_BOOT_FAIL("Ramdisk not found (did you supply enough memory?");
		panic("%! ramdisk not found (did you supply enough memory?)", "kern");
	}
	ramdiskLoadFromModule(ramdiskModule);

	g_address initialPdPhys = setupInformation->initialPageDirectoryPhysical;
	memoryUnmapSetupMemory();

	kernelRunBootstrapCore(initialPdPhys);
	__builtin_unreachable();
}

void kernelRunBootstrapCore(g_physical_address initialPdPhys)
{
	mutexInitialize(&bootstrapCoreLock);
	mutexInitialize(&applicationCoreLock);

	mutexAcquire(&bootstrapCoreLock);

	systemInitializeBsp(initialPdPhys);
	filesystemInitialize();
	pipeInitialize();
	messageInitialize();
	atomicInitialize();
	clockInitialize();

	taskingInitializeBsp();
	syscallRegisterAll();

	taskingAssign(taskingGetLocal(), taskingCreateTask((g_virtual_address) kernelInitializationThread, taskingCreateProcess(), G_SECURITY_LEVEL_KERNEL));

	logInfo("%! starting on %i cores", "kernel", processorGetNumberOfProcessors());
	mutexRelease(&bootstrapCoreLock);

	systemWaitForApplicationCores();

	systemMarkReady();
	interruptsEnable();
	for(;;)
		asm("hlt");
}

void kernelRunApplicationCore()
{
	mutexAcquire(&bootstrapCoreLock);
	mutexRelease(&bootstrapCoreLock);

	mutexAcquire(&applicationCoreLock);

	systemInitializeAp();
	taskingInitializeAp();

	systemMarkApplicationCoreReady();
	mutexRelease(&applicationCoreLock);

	systemWaitForApplicationCores();

	systemWaitForReady();
	interruptsEnable();
	for(;;)
		asm("hlt");
}

void kernelSpawnService(const char* path, const char* args, g_security_level securityLevel)
{
	g_task* currentTask = taskingGetCurrentTask();
	g_fd fd;
	g_fs_open_status open = filesystemOpen(path, G_FILE_FLAG_MODE_READ, currentTask, &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		auto spawnRes = taskingSpawn(currentTask, fd, securityLevel);
		if(spawnRes.status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			spawnRes.process->environment.arguments = args;
			spawnRes.process->main->status = G_THREAD_STATUS_RUNNING;
		}
		else
		{
			logInfo("%! failed to spawn %s with status %i", "kernel", path, spawnRes.status);
		}
	}
	else
	{
		logInfo("%! failed to find %s with status %i", "kernel", path, open);
	}
}

void kernelInitializationThread()
{
	logInfo("%! loading system services", "init");

	G_PRETTY_BOOT_STATUS_P(40);
	kernelSpawnService("/applications/ps2driver.bin", "", G_SECURITY_LEVEL_DRIVER);
	G_PRETTY_BOOT_STATUS_P(60);
	kernelSpawnService("/applications/vbedriver.bin", "", G_SECURITY_LEVEL_DRIVER);

	G_PRETTY_BOOT_STATUS_P(80);
	// kernelSpawnService("/applications/terminal.bin", "--headless", G_SECURITY_LEVEL_DRIVER);
	kernelSpawnService("/applications/windowserver.bin", "", G_SECURITY_LEVEL_APPLICATION);

	taskingExit();
}
