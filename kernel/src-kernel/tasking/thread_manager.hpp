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
	static g_thread* createProcess(g_security_level securityLevel);
	static g_thread* createThread(g_process* process);
	static g_thread* createProcessVm86(uint8_t interrupt, g_vm86_registers& in, g_vm86_registers* out);
	static void prepareThreadLocalStorage(g_thread* thread);

	static void deleteTask(g_thread* task);

	static bool registerTaskForIdentifier(g_thread* task, const char* identifier);
	static g_thread* getTaskForIdentifier(const char* identifier);

private:
	static void applySecurityLevel(g_processor_state* state, g_security_level securityLevel);

	static g_physical_address prepareSpaceForProcess(g_virtual_address kernelStack, g_virtual_address userStack = 0);
	static g_physical_address prepareSpaceForFork(g_thread* current, g_virtual_address kernelStack, g_virtual_address userStack = 0);
	static void prepareSpaceForThread(g_page_directory dir, g_virtual_address kernelStack, g_virtual_address userStack);

	static void dumpTask(g_thread* task);
};

#endif
