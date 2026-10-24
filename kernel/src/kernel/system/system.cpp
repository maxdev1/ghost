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

#include "kernel/system/system.hpp"
#include "kernel/calls/syscall.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/system/acpi/acpi.hpp"
#include "kernel/system/interrupts/apic/apic.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/timing/hpet.hpp"
#include "kernel/system/smp.hpp"
#include "kernel/panic.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/paging.hpp"

static int applicationCoresWaiting;
static bool systemReady = false;

void systemInitializeBsp(g_physical_address rsdp)
{
	processorInitializeBsp();

	acpiInitialize(rsdp);
	apicDetect();
	hpetInitialize();

	if(!processorListAvailable())
		panic("%! no processors found", "system");

	gdtInitialize();
	gdtInitializeLocal();

	interruptsInitializeBsp();
	syscallRegisterAll();

	processorFinalizeSetup();

	auto numCores = processorGetNumberOfProcessors();
	__atomic_store_n(&applicationCoresWaiting, numCores - 1, __ATOMIC_RELEASE);
	if(numCores > 1)
		smpInitialize(pagingGetCurrentSpace());
}

void systemInitializeAp()
{
	gdtInitializeLocal();
	interruptsInitializeAp();
	processorFinalizeSetup();
}

void systemWaitForApplicationCores()
{
	logDebug("%! waiting for %i application processors", processorGetCurrentId() == 0 ? "bsp" : "ap",
	         __atomic_load_n(&applicationCoresWaiting, __ATOMIC_ACQUIRE));
	while(__atomic_load_n(&applicationCoresWaiting, __ATOMIC_ACQUIRE) > 0)
		asm volatile("pause");
}

void systemWaitForReady()
{
	logDebug("%! waiting for system to be ready", processorGetCurrentId() == 0 ? "bsp" : "ap");
	while(!__atomic_load_n(&systemReady, __ATOMIC_ACQUIRE))
		asm volatile("pause");
}

void systemMarkApplicationCoreReady()
{
	__atomic_fetch_sub(&applicationCoresWaiting, 1, __ATOMIC_ACQ_REL);
}

void systemMarkReady()
{
	logInfo("%! ready", "system");
	__atomic_store_n(&systemReady, true, __ATOMIC_RELEASE);
}

bool systemIsReady()
{
	return __atomic_load_n(&systemReady, __ATOMIC_ACQUIRE);
}
