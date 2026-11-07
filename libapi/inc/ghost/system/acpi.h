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

#ifndef GHOST_API_SYSTEM_ACPI
#define GHOST_API_SYSTEM_ACPI

#include "../common.h"
#include "../stdint.h"
#include "../tasks/types.h"
#include "power.h"

__BEGIN_C

#define G_ACPI_DRIVER_NAME "acpidriver"
#define G_ACPI_MAGIC 0x47414350u
#define G_ACPI_VERSION 2
#define G_ACPI_TIMEOUT_MS 30000ULL
#define G_ACPI_MAX_WRITES 8

/**
 * Results returned by ACPI service requests and privileged platform operations.
 */
typedef uint8_t g_acpi_status;
#define G_ACPI_SUCCESSFUL ((g_acpi_status) 0)
#define G_ACPI_BUSY ((g_acpi_status) 1)
#define G_ACPI_NOT_SUPPORTED ((g_acpi_status) 2)
#define G_ACPI_IO_ERROR ((g_acpi_status) 4)
#define G_ACPI_TIMED_OUT ((g_acpi_status) 5)
#define G_ACPI_HARDWARE_ERROR ((g_acpi_status) 6)
#define G_ACPI_NOT_PERMITTED ((g_acpi_status) 7)
#define G_ACPI_INVALID_ARGUMENT ((g_acpi_status) 8)

typedef uint16_t g_acpi_command;
#define G_ACPI_GET_INFO ((g_acpi_command) 1)
#define G_ACPI_PREPARE ((g_acpi_command) 2)
#define G_ACPI_CANCEL ((g_acpi_command) 3)

typedef struct g_acpi_sleep_state
{
	uint8_t supported;
	uint8_t typeA;
	uint8_t typeB;
} __attribute__((packed)) g_acpi_sleep_state;

typedef struct g_acpi_info
{
	uint8_t tablesReady;
	uint8_t namespaceReady;
	uint8_t hardwareReduced;
	g_acpi_sleep_state sleep[6];
} __attribute__((packed)) g_acpi_info;

typedef uint8_t g_acpi_address_space;
#define G_ACPI_MEMORY ((g_acpi_address_space) 0)
#define G_ACPI_IO ((g_acpi_address_space) 1)
#define G_ACPI_PCI ((g_acpi_address_space) 2)

typedef struct g_acpi_register_write
{
	uint64_t address;
	uint64_t value;
	g_acpi_address_space space;
	uint8_t width;
} __attribute__((packed)) g_acpi_register_write;

/**
 * Finite hardware operations prepared by the driver before CPUs are parked.
 * MMIO is mapped and every operation validated before entering kernel commit.
 */
typedef struct g_acpi_power_plan
{
	uint8_t count;
	uint8_t retryStart;
	g_acpi_register_write writes[G_ACPI_MAX_WRITES];
} __attribute__((packed)) g_acpi_power_plan;

typedef struct g_acpi_header
{
	uint32_t magic;
	uint16_t version;
	g_acpi_command command;
} __attribute__((packed)) g_acpi_header;

typedef struct g_acpi_response_header
{
	uint32_t magic;
	uint16_t version;
	g_acpi_command command;
	g_acpi_status status;
} __attribute__((packed)) g_acpi_response_header;

typedef struct g_acpi_get_info_request
{
	g_acpi_header header;
} __attribute__((packed)) g_acpi_get_info_request;

typedef struct g_acpi_power_request
{
	g_acpi_header header;
	uint32_t token;
	g_tid coordinator;
	g_power_action action;
} __attribute__((packed)) g_acpi_power_request;

typedef g_acpi_power_request g_acpi_prepare_request;
typedef g_acpi_power_request g_acpi_cancel_request;

typedef struct g_acpi_get_info_response
{
	g_acpi_response_header header;
	g_acpi_info info;
} __attribute__((packed)) g_acpi_get_info_response;

typedef struct g_acpi_prepare_response
{
	g_acpi_response_header header;
	uint32_t token;
	g_acpi_power_plan plan;
} __attribute__((packed)) g_acpi_prepare_response;

typedef struct g_acpi_cancel_response
{
	g_acpi_response_header header;
	uint32_t token;
} __attribute__((packed)) g_acpi_cancel_response;

/**
 * Returns firmware-table and interrupt-controller information to a driver.
 */
typedef struct g_syscall_acpi_platform_info
{
	uint64_t rsdp;
	uint8_t ioapic;
	g_acpi_status status;
} __attribute__((packed)) g_syscall_acpi_platform_info;

/**
 * Maps a physical ACPI region for a driver and returns its virtual address.
 */
typedef struct g_syscall_acpi_map
{
	uint64_t physicalAddress;
	uint32_t size;
	uint64_t virtualAddress;
	g_acpi_status status;
} __attribute__((packed)) g_syscall_acpi_map;

__END_C

#endif
