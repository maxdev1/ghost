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

#include "kernel/system/power/acpi_power.hpp"
#include <ghost/system/acpi.h>
#include <ghost/system/pci.h>
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/acpi_driver.hpp"
#include "kernel/system/io_port.hpp"
#include "kernel/system/pci_driver.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"

static g_acpi_info powerAcpiInfo;
static g_acpi_power_plan powerAcpiPlan;
static g_virtual_address powerAcpiMappings[G_ACPI_MAX_WRITES];
static uint32_t powerAcpiToken;
static uint64_t powerAcpiTicksPerMs;
static bool powerAcpiPrepared;

static void powerAcpiSleep(uint64_t milliseconds)
{
	auto task = taskingGetCurrentTask();
	taskingWait(task, __func__,
				[task, milliseconds]() { clockWaitForTime(task->id, clockGetMilliseconds() + milliseconds); });
}

static void powerAcpiStall(uint64_t milliseconds)
{
	auto start = processorReadTsc();
	while(processorReadTsc() - start < powerAcpiTicksPerMs * milliseconds)
		asm volatile("pause");
}

static void powerAcpiCalibrateStall()
{
	auto start = clockGetMilliseconds();
	auto before = processorReadTsc();

	powerAcpiSleep(10);

	auto elapsed = clockGetMilliseconds() - start;
	powerAcpiTicksPerMs = (processorReadTsc() - before) / (elapsed ? elapsed : 1);

	if(!powerAcpiTicksPerMs)
		powerAcpiTicksPerMs = 1;
}

void powerAcpiInitialize()
{
	powerAcpiCalibrateStall();

	if(!acpiDriverGetInfo(&powerAcpiInfo))
		logWarn("%! ACPI driver unavailable", "power");
}

bool powerAcpiSupported(g_power_action action)
{
	return action == G_POWER_REBOOT || (powerAcpiInfo.namespaceReady && powerAcpiInfo.sleep[5].supported);
}

static void powerAcpiReleasePlan()
{
	for(auto& address: powerAcpiMappings)
		if(address)
		{
			pagingUnmapPage(address);
			addressRangePoolFree(memoryVirtualRangePool, address);
			address = 0;
		}
	powerAcpiPlan = {};
}

static bool powerAcpiValidatePciWrite(const g_acpi_register_write& write, g_power_action action, uint8_t count)
{
	return action == G_POWER_REBOOT && count == 1 && write.width == 1 && !(write.address >> 40) &&
		   ((write.address >> 32) & 255) <= 31 && ((write.address >> 16) & 0xffff) <= 7 &&
		   (write.address & 0xffff) <= 255;
}

static bool powerAcpiValidateWrite(const g_acpi_register_write& write, g_power_action action, uint8_t count)
{
	if(write.width != 1 && write.width != 2 && write.width != 4 && write.width != 8)
		return false;
	if(write.width < 8 && write.value >> (write.width * 8))
		return false;

	switch(write.space)
	{
		case G_ACPI_PCI:
			return powerAcpiValidatePciWrite(write, action, count);
		case G_ACPI_IO:
			return write.width <= 4 && write.address <= 0x10000u - write.width && !(write.address & (write.width - 1));
		case G_ACPI_MEMORY:
			return !(write.address & (write.width - 1)) && memoryPhysicalIsRangeAddressable(write.address, write.width);
		default:
			return false;
	}
}

static bool powerAcpiValidatePlan(const g_acpi_power_plan& plan, g_power_action action)
{
	if(plan.count > G_ACPI_MAX_WRITES || (plan.count && plan.retryStart >= plan.count) ||
	   (action == G_POWER_SHUTDOWN && !plan.count))
		return false;

	for(unsigned i = 0; i < plan.count; ++i)
		if(!powerAcpiValidateWrite(plan.writes[i], action, plan.count))
			return false;

	return true;
}

static g_virtual_address powerAcpiMapRegister(uint64_t physical)
{
	auto address = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	if(!address)
		return 0;
	if(!pagingMapPage(address, G_PAGE_ALIGN_DOWN(physical), G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_UNCACHED))
	{
		addressRangePoolFree(memoryVirtualRangePool, address);
		return 0;
	}

	return address;
}

static bool powerAcpiMapPlan(const g_acpi_power_plan& plan, g_power_action action)
{
	if(!powerAcpiValidatePlan(plan, action))
		return false;

	for(unsigned i = 0; i < plan.count; ++i)
	{
		if(plan.writes[i].space != G_ACPI_MEMORY)
			continue;

		powerAcpiMappings[i] = powerAcpiMapRegister(plan.writes[i].address);
		if(!powerAcpiMappings[i])
		{
			powerAcpiReleasePlan();
			return false;
		}
	}

	powerAcpiPlan = plan;
	return true;
}

bool powerAcpiPrepare(g_power_action action)
{
	powerAcpiReleasePlan();

	if(++powerAcpiToken == 0)
		++powerAcpiToken;

	powerAcpiPrepared = true;

	g_acpi_power_plan plan{};
	if(!acpiDriverPrepare(action, powerAcpiToken, &plan))
		return false;

	if(!powerAcpiMapPlan(plan, action))
	{
		logWarn("%! ACPI power register plan rejected: writes %i retry start %i", "power", plan.count,
				plan.retryStart);
		powerAcpiReleasePlan();
		return false;
	}

	return true;
}

void powerAcpiCancel(g_power_action action)
{
	powerAcpiReleasePlan();
	if(!powerAcpiPrepared)
		return;

	if(!acpiDriverCancel(action, powerAcpiToken))
		logWarn("%! ACPI driver rollback failed", "power");

	powerAcpiPrepared = false;
}

void powerAcpiTryPciReboot()
{
	if(powerAcpiPlan.count != 1 || powerAcpiPlan.writes[0].space != G_ACPI_PCI)
		return;

	auto write = powerAcpiPlan.writes[0];
	auto device = G_PCI_DEVICE_ADDRESS_BUILD(0, (write.address >> 32) & 255, (write.address >> 16) & 255);
	pciDriverWrite(device, write.address & 255, 1, write.value);
	powerAcpiSleep(1000);
}

static void powerAcpiWriteIo(const g_acpi_register_write& write)
{
	switch(write.width)
	{
		case 1:
			ioPortWriteByte(write.address, write.value);
			break;
		case 2:
			ioPortWriteShort(write.address, write.value);
			break;
		case 4:
			ioPortWriteInt(write.address, write.value);
			break;
	}
}

static void powerAcpiWriteMemory(const g_acpi_register_write& write, g_virtual_address mapping)
{
	auto address = mapping + (write.address & (G_PAGE_SIZE - 1));
	switch(write.width)
	{
		case 1:
			*(volatile uint8_t*) address = write.value;
			break;
		case 2:
			*(volatile uint16_t*) address = write.value;
			break;
		case 4:
			*(volatile uint32_t*) address = write.value;
			break;
		case 8:
			*(volatile uint64_t*) address = write.value;
			break;
	}
}

static void powerAcpiWriteRegisters(unsigned first)
{
	for(unsigned i = first; i < powerAcpiPlan.count; ++i)
	{
		const auto& write = powerAcpiPlan.writes[i];
		switch(write.space)
		{
			case G_ACPI_IO:
				powerAcpiWriteIo(write);
				break;
			case G_ACPI_MEMORY:
				powerAcpiWriteMemory(write, powerAcpiMappings[i]);
				break;
		}
	}
}

static void powerAcpiTryKeyboardReboot()
{
	for(unsigned i = 0; i < 10000; ++i)
	{
		if(!(ioPortReadByte(0x64) & 2))
		{
			ioPortWriteByte(0x64, 0xfe);
			powerAcpiStall(100);
			return;
		}

		asm volatile("pause");
	}
}

void powerAcpiPerform(g_power_action action)
{
	powerAcpiWriteRegisters(0);
	powerAcpiStall(action == G_POWER_SHUTDOWN ? 10000 : 1000);
	if(action == G_POWER_SHUTDOWN)
	{
		powerAcpiWriteRegisters(powerAcpiPlan.retryStart);
		return;
	}

	powerAcpiTryKeyboardReboot();
}
