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

#ifndef GHOST_MULTITASKING_THREAD
#define GHOST_MULTITASKING_THREAD

#include <system/processor_state.hpp>

#include "ghost/kernel.h"
#include "ghost/calls/calls.h"
#include "ghost/signal.h"
#include "memory/paging.hpp"
#include "memory/collections/address_range_pool.hpp"

// forward declarations
class g_process;
class g_waiter;
class g_scheduler;

/**
 * Data used by virtual 8086 processes
 */
struct g_thread_information_vm86 {
	g_thread_information_vm86() :
			cpuIf(false), out(0), interruptRecursionLevel(0) {
	}

	bool cpuIf;
	g_vm86_registers* out;
	uint32_t interruptRecursionLevel;
};

/**
 *
 */
enum class g_thread_interruption_info_type
	: uint8_t {
		NONE, IRQ, SIGNAL
};

/**
 *
 */
class g_thread_interruption_info {
public:
	g_processor_state cpuState;
	g_processor_state* cpuStateAddress;
	g_waiter* waitManager;

	g_thread_interruption_info_type type = g_thread_interruption_info_type::NONE;
	uint8_t handled_irq;
	int handled_signal;
};

/**
 *
 */
class g_thread {
private:
	g_thread_information_vm86* vm86Information;
	char* identifier;

public:
	g_thread(g_thread_type _type);
	~g_thread();

	g_tid id;
	bool alive;
	g_thread_type type;
	g_thread_priority priority;
	g_process* process;
	g_scheduler* scheduler;

	g_waiter* waitManager;
	uint32_t waitCount;

	uint64_t rounds;

	void* userData;
	void* threadEntry;

	g_processor_state* cpuState;
	g_virtual_address kernelStackPageVirt;
	g_virtual_address kernelStackEsp0;
	g_virtual_address userStackAreaStart;
	uint8_t userStackPages;

	g_virtual_address user_thread_addr;
	g_virtual_address tls_copy_virt;

	g_thread_interruption_info* interruption_info;

	g_thread_information_vm86* getVm86Information();
	const char* getIdentifier();
	void setIdentifier(const char* newIdentifier);

	void wait(g_waiter* waitManager);
	void unwait();
	bool checkWaiting();

	void raise_signal(int signal);
	void enter_irq_handler(uintptr_t address, uint8_t irq, uintptr_t callback);
	void enter_signal_handler(uintptr_t address, int signal, uintptr_t callback);
	bool start_prepare_interruption();
	void finish_prepare_interruption(uintptr_t address, uintptr_t callback);

	void store_for_interruption();
	void restore_interrupted_state();
};

#endif
