/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "power.hpp"
#include <cstdio>
#include <ghost.h>
#include <uacpi/event.h>
#include <uacpi/kernel_api.h>
#include <uacpi/registers.h>
#include <uacpi/sleep.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>
extern "C"
{
#include <uacpi/internal/event.h>
}
#include <uacpi/acpi.h>
#include <uacpi/utilities.h>

static g_acpi_info acpiInfo;
static acpi_fadt* acpiFadt;

static bool acpiPowerCheckStatus(const char* stage, uacpi_status status)
{
	if(status == UACPI_STATUS_OK)
		return true;

	klog("acpi: %s failed: %s (%u)", stage, uacpi_status_to_string(status), unsigned(status));
	return false;
}

static g_acpi_sleep_state acpiPowerReadSleepState(unsigned state)
{
	char path[] = "\\_S0";
	path[3] += state;
	uacpi_object* object = nullptr;

	g_acpi_sleep_state result{};
	if(uacpi_eval_simple_typed(nullptr, path, UACPI_OBJECT_PACKAGE_BIT, &object) != UACPI_STATUS_OK)
		return result;

	uacpi_object_array package{};
	if(uacpi_object_get_package(object, &package) == UACPI_STATUS_OK && package.count)
	{
		uacpi_u64 a = 0;
		uacpi_u64 b = 0;
		bool valid = uacpi_object_get_integer(package.objects[0], &a) == UACPI_STATUS_OK;
		b = a >> 8;
		if(package.count > 1)
			valid = valid && uacpi_object_get_integer(package.objects[1], &b) == UACPI_STATUS_OK;
		if(valid && (a & 0xff) <= ACPI_SLP_TYP_MAX && b <= ACPI_SLP_TYP_MAX)
			result = {1, uint8_t(a), uint8_t(b)};
	}

	uacpi_object_unref(object);
	return result;
}

g_acpi_info acpiPowerInitialize(bool ioapic)
{
	auto status = uacpi_initialize(UACPI_FLAG_BAD_CSUM_FATAL | UACPI_FLAG_BAD_TBL_SIGNATURE_FATAL);
	acpiInfo.tablesReady = status == UACPI_STATUS_OK && uacpi_table_fadt(&acpiFadt) == UACPI_STATUS_OK;
	acpiInfo.hardwareReduced = acpiInfo.tablesReady && (acpiFadt->flags & ACPI_HW_REDUCED_ACPI);

	if(status == UACPI_STATUS_OK)
		status = uacpi_namespace_load();
	if(status == UACPI_STATUS_OK)
		status = uacpi_set_interrupt_model(ioapic ? UACPI_INTERRUPT_MODEL_IOAPIC : UACPI_INTERRUPT_MODEL_PIC);
	if(status == UACPI_STATUS_OK)
		status = uacpi_namespace_initialize();

	acpiInfo.namespaceReady = status == UACPI_STATUS_OK;

	if(acpiInfo.namespaceReady)
	{
		for(unsigned state = 0; state < 6; ++state)
			acpiInfo.sleep[state] = acpiPowerReadSleepState(state);
	}
	else
		klog("acpi: initialization failed: %s", uacpi_status_to_string(status));

	return acpiInfo;
}

static bool acpiPowerAddWrite(g_acpi_power_plan* plan, const acpi_gas& gas, uint8_t width, uint64_t value)
{
	if(!gas.address || gas.register_bit_offset || plan->count >= G_ACPI_MAX_WRITES ||
	   (gas.address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_IO &&
		gas.address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_MEMORY) ||
	   (width != 1 && width != 2 && width != 4 && width != 8) ||
	   (gas.address_space_id == UACPI_ADDRESS_SPACE_SYSTEM_IO && (width > 4 || gas.address > 0x10000u - width)) ||
	   (gas.address & (width - 1)))
	{
		klog("acpi: invalid power register: space %u address %llx offset %u width %u writes %u",
			 unsigned(gas.address_space_id), (unsigned long long) gas.address, unsigned(gas.register_bit_offset),
			 unsigned(width), unsigned(plan->count));
		return false;
	}
	plan->writes[plan->count++] = {gas.address, value,
								   gas.address_space_id == UACPI_ADDRESS_SPACE_SYSTEM_IO ? G_ACPI_IO : G_ACPI_MEMORY,
								   width};
	return true;
}

static bool acpiPowerQuiesceEvents()
{
	return acpiPowerCheckStatus("disable GPEs", uacpi_disable_all_gpes()) &&
		   acpiPowerCheckStatus("drain ACPI work", uacpi_kernel_wait_for_work_completion());
}

static bool acpiPowerAddControlWrites(g_acpi_power_plan* plan, uint64_t a, uint64_t b)
{
	if(!acpiPowerAddWrite(plan, acpiFadt->x_pm1a_cnt_blk, 2, a))
		return false;
	return !acpiFadt->x_pm1b_cnt_blk.address || acpiPowerAddWrite(plan, acpiFadt->x_pm1b_cnt_blk, 2, b);
}

/**
 * AML and event preparation finish in the driver. Only the fixed S5 register
 * sequence crosses into the kernel, where it can run after CPU/cache shutdown.
 * Mirrors uACPI's split SLP_TYP/SLP_EN writes, including optional PM1b control.
 */
static bool acpiPowerPrepareShutdown(g_acpi_power_plan* plan)
{
	if(!acpiInfo.namespaceReady || !acpiInfo.sleep[5].supported)
	{
		klog("acpi: S5 preparation unavailable: namespace %u S5 %u", unsigned(acpiInfo.namespaceReady),
			 unsigned(acpiInfo.sleep[5].supported));
		return false;
	}

	if(!acpiPowerCheckStatus("prepare S5 firmware (_PTS)", uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5)))
		return false;

	if(!acpiPowerQuiesceEvents())
		return false;

	if(acpiInfo.hardwareReduced)
	{
		if(!acpiPowerCheckStatus("clear hardware-reduced wake status",
								 uacpi_write_register_field(UACPI_REGISTER_FIELD_HWR_WAK_STS, 1)))
			return false;
		return acpiPowerAddWrite(plan, acpiFadt->sleep_control_reg, 1,
								 (acpiInfo.sleep[5].typeA << ACPI_SLP_CNT_SLP_TYP_IDX) | ACPI_SLP_CNT_SLP_EN_MASK);
	}

	if(!acpiPowerCheckStatus("clear wake status", uacpi_write_register_field(UACPI_REGISTER_FIELD_WAK_STS, 1)) ||
	   !acpiPowerCheckStatus("clear ACPI events", uacpi_clear_all_events()) ||
	   !acpiPowerCheckStatus("enable wake GPEs", uacpi_enable_all_wake_gpes()))
		return false;

	uacpi_u64 control = 0;
	if(!acpiPowerCheckStatus("read PM1 control", uacpi_read_register(UACPI_REGISTER_PM1_CNT, &control)))
		return false;

	control &= ~(uint64_t(ACPI_PM1_CNT_SLP_TYP_MASK | ACPI_PM1_CNT_SLP_EN_MASK));
	auto a = control | (uint64_t(acpiInfo.sleep[5].typeA) << ACPI_PM1_CNT_SLP_TYP_IDX);
	auto b = control | (uint64_t(acpiInfo.sleep[5].typeB) << ACPI_PM1_CNT_SLP_TYP_IDX);
	if(!acpiPowerAddControlWrites(plan, a, b))
		return false;

	plan->retryStart = plan->count;
	return acpiPowerAddControlWrites(plan, a | ACPI_PM1_CNT_SLP_EN_MASK, b | ACPI_PM1_CNT_SLP_EN_MASK);
}

static bool acpiPowerPrepareReboot(g_acpi_power_plan* plan)
{
	if(acpiInfo.namespaceReady && !acpiPowerQuiesceEvents())
		return false;

	if(!acpiInfo.tablesReady || !(acpiFadt->flags & ACPI_RESET_REG_SUP) || !acpiFadt->reset_reg.address)
		return true;

	const auto& gas = acpiFadt->reset_reg;
	if(gas.address_space_id == UACPI_ADDRESS_SPACE_PCI_CONFIG)
	{
		plan->count = 1;
		plan->writes[0] = {gas.address, acpiFadt->reset_value, G_ACPI_PCI, 1};
		return true;
	}

	if(gas.access_size > 4)
	{
		klog("acpi: invalid reset register access size %u", unsigned(gas.access_size));
		return false;
	}

	uint8_t width = 1;
	if(gas.address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_IO)
		width = gas.access_size ? uint8_t(1 << (gas.access_size - 1)) : uint8_t((gas.register_bit_width + 7) / 8);

	return acpiPowerAddWrite(plan, gas, width, acpiFadt->reset_value);
}

bool acpiPowerPrepare(g_power_action action, g_acpi_power_plan* plan)
{
	*plan = {};
	switch(action)
	{
		case G_POWER_SHUTDOWN:
			return acpiPowerPrepareShutdown(plan);
		case G_POWER_REBOOT:
			return acpiPowerPrepareReboot(plan);
		default:
			return false;
	}
}

void acpiPowerCancel(g_power_action action)
{
	if(!acpiInfo.namespaceReady)
		return;
	if(action == G_POWER_SHUTDOWN)
	{
		acpiPowerCheckStatus("prepare S5 rollback", uacpi_prepare_for_wake_from_sleep_state(UACPI_SLEEP_STATE_S5));
		acpiPowerCheckStatus("restore firmware after S5 rollback", uacpi_wake_from_sleep_state(UACPI_SLEEP_STATE_S5));
	}
	acpiPowerCheckStatus("restore runtime GPEs", uacpi_enable_all_runtime_gpes());
}
