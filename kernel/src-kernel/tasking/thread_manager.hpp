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

#ifndef GHOSTKERNEL_TASKING_THREADMANAGER
#define GHOSTKERNEL_TASKING_THREADMANAGER

#include <system/processor_state.hpp>

#include "ghost/kernel.h"
#include "ghost/stdint.h"
#include <tasking/thread.hpp>

/**
 *
 */
class g_thread_manager {
public:
	static g_thread* fork(g_thread* task);

	static g_thread* createProcess(g_security_level securityLevel, g_process* parent);
	static g_thread* createSubThread(g_process* process);

	static g_thread* createProcessVm86(uint8_t interrupt, g_vm86_registers& in, g_vm86_registers* out);

	static void prepareThreadLocalStorage(g_thread* thread);

	static void deleteTask(g_thread* task);

	static bool registerTaskForIdentifier(g_thread* task, const char* identifier);
	static g_thread* getTaskForIdentifier(const char* identifier);

	static g_virtual_address getMemoryUsage(g_thread* task);

private:
	static void applySecurityLevel(g_processor_state* state, g_security_level securityLevel);

	static g_thread* createThread(g_process* process, g_thread_type type);

	/**
	 * Allocates and initializes a new page directory used for process creation.
	 */
	static g_page_directory initializePageDirectoryForProcess();

	static g_physical_address forkCurrentPageDirectory(g_process* process, g_thread* current, g_virtual_address* outKernelStackVirt,
			g_virtual_address* outUserStackVirt);

	/**
	 * Allocates the user thread stack for a new thread in the given process.
	 */
	static bool createThreadUserStack(g_process* process, g_virtual_address* outUserStackVirt);

	/**
	 * Allocates the kernel thread stack for a new thread in the given process.
	 */
	static bool createThreadKernelStack(g_process* process, g_virtual_address* outKernelStackVirt);

	static void dumpTask(g_thread* task);
};

#endif
