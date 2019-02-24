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

#ifndef __KERNEL_ACPI__
#define __KERNEL_ACPI__

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "ghost/types.h"
#include "kernel/system/acpi/rsdp.hpp"

// Set a maximum for SDTs to avoid problems with possible junk tables
#define	G_SDT_MAXIMUM_BYTES		0x10000

struct g_acpi_table_header
{
	uint8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oemId[6];
	uint8_t oemTableId[8];
	uint32_t oemRevision;
	uint32_t creatorId;
	uint32_t creatorRevision;
}__attribute__((packed));

struct g_acpi_entry
{
	g_acpi_table_header* header;
	g_acpi_entry* next;
};

/**
 *
 */
void acpiInitialize();

/**
 * Prepares the root SDT (RSTD or XSDT) by mapping the required
 * memory into the virtual address space.
 */
void acpiPrepareRootSDT(g_rsdp_descriptor* rsdp);

/**
 * Temporarily maps the SDT at the given location to the virtual address space,
 * retrieves its length and then unmaps it again.
 *
 * @return the length of the table or 0 if failed
 */
uint32_t acpiGetLengthOfUnmappedSDT(g_physical_address tableLocation);

/**
 * Maps the table at the given physical address into the virtual space.
 * First uses the {getLengthOfUnmappedSDT} to get the length of the table
 * and then maps the required size in the address space.
 *
 * @return header of the mapped table or 0
 */
g_acpi_table_header* acpiMapSDT(g_physical_address tableLocation);

/**
 * Validates the whole system descriptor table with the given header
 */
bool acpiValidateSDT(g_acpi_table_header* header);

/**
 * Returns the number of entries that the RSDT with the given header has
 */
uint32_t acpiGetRSDTentryCount();

/**
 * Returns the RSDT entry at the given index.
 */
g_physical_address acpiGetRSDTentry(uint32_t index);

/**
 *
 */
g_acpi_entry* acpiGetEntryWithSignature(const char* signature);

/**
 *
 */
bool acpiEntryHasSignature(g_acpi_entry* entry, const char* str);

#endif
