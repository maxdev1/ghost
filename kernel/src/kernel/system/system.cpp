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
#include "kernel/memory/gdt.hpp"
#include "kernel/system/acpi/acpi.hpp"
#include "kernel/system/interrupts/apic/apic.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/smp.hpp"
#include "shared/panic.hpp"

static int applicationCoresWaiting;
static bool bspInitialized = false;
static bool systemReady = false;

void systemInitializeBsp(g_physical_address initialPdPhys)
{
	processorInitializeBsp();

	acpiInitialize();
	apicDetect();

	if(!processorListAvailable())
		panic("%! no processors found", "system");

	gdtPrepare();
	gdtInitialize();

	interruptsInitializeBsp();

	auto numCores = processorGetNumberOfProcessors();
	if(numCores > 1)
		smpInitialize(initialPdPhys);

	applicationCoresWaiting = numCores - 1;
	bspInitialized = true;
}

void systemInitializeAp()
{
	processorInitializeAp();

	gdtInitialize();
	interruptsInitializeAp();
}

void systemWaitForApplicationCores()
{
	logDebug("%! waiting for %i application processors", processorGetCurrentId() == 0 ? "bsp" : "ap", applicationCoresWaiting);
	while(applicationCoresWaiting > 0)
		asm("pause");
}

void systemWaitForReady()
{
	logDebug("%! waiting for system to be ready", processorGetCurrentId() == 0 ? "bsp" : "ap");
	while(!systemReady)
		asm("pause");
}

void systemMarkApplicationCoreReady()
{
	--applicationCoresWaiting;
}

void systemMarkReady()
{
	logInfo("%! ready", "system");
	systemReady = true;
}

bool systemIsReady()
{
	return systemReady;
}
