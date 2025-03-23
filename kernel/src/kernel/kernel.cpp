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
#include <shared/memory/constants.hpp>
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
#include "shared/video/console_video.hpp"
#include "shared/video/pretty_boot.hpp"
#include "shared/logger/logger.hpp"
#include "shared/boot/limine.hpp"

static g_mutex bootstrapCoreLock;
static g_mutex applicationCoreLock;

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
volatile limine_framebuffer_request framebufferRequest = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
};

__attribute__((used, section(".limine_requests")))
volatile limine_memmap_request memoryMapRequest = {
		.id = LIMINE_MEMMAP_REQUEST,
		.revision = 0
};

__attribute__((used, section(".limine_requests")))
volatile limine_paging_mode_request pagingModeRequest = {
		.id = LIMINE_PAGING_MODE_REQUEST,
		.revision = 0,
		.mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

__attribute__((used, section(".limine_requests")))
volatile limine_module_request moduleRequest = {
		.id = LIMINE_MODULE_REQUEST,
		.revision = 0,
};

__attribute__((used, section(".limine_requests")))
volatile limine_rsdp_request rsdpRequest = {
		.id = LIMINE_RSDP_REQUEST,
		.revision = 0,
};

__attribute__((used, section(".limine_requests")))
volatile limine_hhdm_request hhdmRequest = {
		.id = LIMINE_HHDM_REQUEST,
		.revision = 0,
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void failEarly()
{
	asm("cli; hlt;");
}

extern "C" void kernelMain()
{
	if(LIMINE_BASE_REVISION_SUPPORTED == false)
		failEarly();

	if(framebufferRequest.response == nullptr || framebufferRequest.response->framebuffer_count < 1)
		failEarly();

	consoleVideoInitialize(framebufferRequest.response->framebuffers[0]);
	if(G_PRETTY_BOOT)
		prettyBootEnable(false);
	else
		consoleVideoClear();
	kernelLoggerInitialize();

	if(pagingModeRequest.response->mode != LIMINE_PAGING_MODE_X86_64_4LVL)
		panic("%! bootloader failed to boot in 4-level paging mode", "error");

	// TODO Just use the value instead of failing if its different:
	if(hhdmRequest.response->offset != G_MEM_HIGHER_HALF_DIRECT_MAP_OFFSET)
		panic("%! higher half mapping not at expected offset %x but %x", "error", G_MEM_HIGHER_HALF_DIRECT_MAP_OFFSET,
		      hhdmRequest.response->offset);

	memoryInitialize(memoryMapRequest.response);

	auto ramdiskFile = limineFindModule(moduleRequest.response, "/boot/ramdisk");
	if(!ramdiskFile)
	{
		G_PRETTY_BOOT_FAIL("Ramdisk file not found");
		panic("%! ramdisk file not found", "kern");
	}
	ramdiskLoadFromBootloaderFile(ramdiskFile);

	// TODO unmap loader memory
	kernelRunBootstrapCore();
	__builtin_unreachable();
}

void kernelRunBootstrapCore()
{
	mutexInitializeGlobal(&bootstrapCoreLock, __func__);
	mutexInitializeGlobal(&applicationCoreLock, __func__);

	mutexAcquire(&bootstrapCoreLock);

	systemInitializeBsp((g_physical_address) rsdpRequest.response->address);
	clockInitialize();
	filesystemInitialize();
	pipeInitialize();
	messageQueuesInitialize();
	messageTopicsInitialize();
	userMutexInitialize();

	taskingInitializeBsp();
	logInfo("%! starting on %i core(s)", "kernel", processorGetNumberOfProcessors());
	mutexRelease(&bootstrapCoreLock);

	auto initializationProcess = taskingCreateProcess(G_SECURITY_LEVEL_KERNEL);
	auto initializationTask = taskingCreateTask((g_virtual_address) kernelInitializationThread, initializationProcess,
	                                            G_SECURITY_LEVEL_KERNEL);
	taskingAssign(taskingGetLocal(), initializationTask);

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

	logInfo("AP core running");
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
	// G_PRETTY_BOOT_STATUS_P(80);
	// kernelSpawnService("/applications/terminal.bin", "--headless", G_SECURITY_LEVEL_DRIVER);

	G_PRETTY_BOOT_STATUS("initializing...", 100);

	// logInfo("do stuff...");
	// for(;;)
	// {
	// 	auto task = taskingGetCurrentTask();
	// 	taskingWait(task, __func__, [task]()
	// 	{
	// 		clockWaitForTime(task->id, clockGetLocal()->time + 1000);
	// 	});
	// 	logInfo("alive...");
	// }
	taskingExit();
}
