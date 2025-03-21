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
#include "kernel/ipc/message_queues.hpp"
#include "kernel/ipc/message_topics.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/logger/kernel_logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/user_mutex.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/system/mutex.hpp"
#include "shared/panic.hpp"
#include "shared/setup_information.hpp"
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"
#include "shared/logger/logger.hpp"

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
	mutexInitializeGlobal(&bootstrapCoreLock, __func__);
	mutexInitializeGlobal(&applicationCoreLock, __func__);

	mutexAcquire(&bootstrapCoreLock);

	systemInitializeBsp(initialPdPhys);
	clockInitialize();
	filesystemInitialize();
	pipeInitialize();
	messageQueuesInitialize();
	messageTopicsInitialize();
	userMutexInitialize();

	taskingInitializeBsp();
	logInfo("%! starting on %i cores", "kernel", processorGetNumberOfProcessors());
	mutexRelease(&bootstrapCoreLock);

	systemWaitForApplicationCores();
	systemMarkReady();

	auto initializationProcess = taskingCreateProcess(G_SECURITY_LEVEL_KERNEL);
	auto initializationTask = taskingCreateTask((g_virtual_address) kernelInitializationThread, initializationProcess,
	                                            G_SECURITY_LEVEL_KERNEL);
	taskingAssign(taskingGetLocal(), initializationTask);

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
	g_fd fd;
	g_fs_open_status open = filesystemOpen(path, G_FILE_FLAG_MODE_READ, taskingGetCurrentTask(), &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		auto spawnRes = taskingSpawn(fd, securityLevel);
		if(spawnRes.status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			spawnRes.process->environment.arguments = args;
			spawnRes.process->main->status = G_TASK_STATUS_RUNNING;
			logInfo("%! %s started in process %i", "service", path, spawnRes.process->id);
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

	G_PRETTY_BOOT_STATUS_P(20);
	kernelSpawnService("/applications/pcidriver.bin", "", G_SECURITY_LEVEL_DRIVER);
	G_PRETTY_BOOT_STATUS_P(20);
	kernelSpawnService("/applications/devicemanager.bin", "", G_SECURITY_LEVEL_DRIVER);
	G_PRETTY_BOOT_STATUS_P(40);
	kernelSpawnService("/applications/ps2driver.bin", "", G_SECURITY_LEVEL_DRIVER);

	G_PRETTY_BOOT_STATUS_P(80);
	kernelSpawnService("/applications/windowserver.bin", "", G_SECURITY_LEVEL_APPLICATION);

	G_PRETTY_BOOT_STATUS("initializing...", 100);

	taskingExit();
}
