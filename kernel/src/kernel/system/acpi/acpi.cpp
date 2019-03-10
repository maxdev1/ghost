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

#include "ghost/types.h"

#include "kernel/system/acpi/acpi.hpp"
#include "kernel/system/acpi/madt.hpp"

#include "kernel/kernel.hpp"
#include "kernel/memory/memory.hpp"

#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

static g_acpi_table_header* acpiRoot = 0;
static g_acpi_entry* acpiTables = 0;
static bool acpiRootIsXsdt = false;

g_acpi_entry* acpiGetEntryWithSignature(const char* signature) {
	g_acpi_entry* cur = acpiTables;
	while (cur) {
		if (acpiEntryHasSignature(cur, signature)) {
			return cur;
		}
		cur = cur->next;
	}
	return 0;
}

void acpiInitialize() {
	g_rsdp_descriptor* rsdp = rsdpFind();
	if (!rsdp)
		kernelPanic("%! failed to find RSDP", "acpi");

	acpiPrepareRootSDT(rsdp);

	for (uint32_t i = 0; i < acpiGetRSDTentryCount(); i++) {
		g_physical_address entry = acpiGetRSDTentry(i);
		if (entry != 0) {
			g_acpi_table_header* sdt = acpiMapSDT(entry);

			// Could not be mapped? Skip
			if (sdt == 0) {
				continue;
			}

			// Create the entry
			g_acpi_entry* entry = new g_acpi_entry();
			entry->header = sdt;
			entry->next = acpiTables;
			acpiTables = entry;
		}
	}

	// Find and parse MADT
	g_acpi_entry* madt = acpiGetEntryWithSignature("APIC");
	if (!madt)
		kernelPanic("%! no MADT table was found in ACPI tables", "acpi");

	madtParse(madt->header);
}

void acpiPrepareRootSDT(g_rsdp_descriptor* rsdp) {

	g_physical_address rootTableLocation = 0;

	acpiRootIsXsdt = false;

	// If ACPI 2.0 or higher, try to use the XSDT
	if (rsdp->revision > 0) {
		g_rsdp_descriptor_20* rsdp20 = (g_rsdp_descriptor_20*) rsdp;
		if (rsdp20->xsdtAddress != 0) {
#if _ARCH_X86_64_
			rootTableLocation = rsdp20->xsdtAddress;
			g_log_debug("%! found XSDT in 64bit range", "acpi");
			acpiRootIsXsdt = true;
#elif _ARCH_X86_
			if(rsdp20->xsdtAddress < 0xFFFFFFFF)
			{
				acpiRootIsXsdt = true;
				logDebug("%! found XSDT in 32bit range", "acpi");
				rootTableLocation = rsdp20->xsdtAddress;
			} else
			{
				acpiRootIsXsdt = false;
				logDebug("%! found XSDT, but range too high for 32bits, attempting to use RSDT", "acpi");
			}
#endif
		}
	}

	// No XSDT? Use RSDT
	if (!acpiRootIsXsdt) {
		rootTableLocation = rsdp->rsdtAddress;
	}

	if (!rootTableLocation)
		kernelPanic("%! failed to find ACPI root table", "acpi");

	g_acpi_table_header* header = acpiMapSDT(rootTableLocation);
	if (!header)
		kernelPanic("%! could not map root system descriptor table", "acpi");

	acpiRoot = header;
}

bool acpiValidateSDT(g_acpi_table_header* header) {
	uint8_t sum = 0;
	uint8_t* tableBytes = reinterpret_cast<uint8_t*>(header);

	for (uint32_t i = 0; i < header->length; i++)
		sum += tableBytes[i];

	return sum == 0;
}

uint32_t acpiGetLengthOfUnmappedSDT(g_physical_address tableLocation) {
	g_physical_address physStart = G_PAGE_ALIGN_DOWN(tableLocation);
	g_virtual_address virtualBase = addressRangePoolAllocate(
			memoryVirtualRangePool, 2);

	if (!pagingMapPage(virtualBase, physStart, DEFAULT_KERNEL_TABLE_FLAGS,
			DEFAULT_KERNEL_PAGE_FLAGS)) {
		logWarn("%! could not create virtual mapping (1) for SDT %h", "acpi",
				tableLocation);
		return 0;
	}
	if (!pagingMapPage(virtualBase + G_PAGE_SIZE, physStart + G_PAGE_SIZE,
			DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS)) {
		logWarn("%! could not create virtual mapping (2) for SDT %h", "acpi",
				tableLocation);
		return 0;
	}

	uint32_t mappingOffset = tableLocation - physStart;

	g_acpi_table_header* header = (g_acpi_table_header*) (virtualBase
			+ mappingOffset);
	uint32_t length = header->length;

	pagingUnmapPage(virtualBase);
	pagingUnmapPage(virtualBase + G_PAGE_SIZE);
	addressRangePoolFree(memoryVirtualRangePool, virtualBase);

	return length;
}

g_acpi_table_header* acpiMapSDT(g_physical_address tableLocation) {
	uint32_t tableLength = acpiGetLengthOfUnmappedSDT(tableLocation);
	if (tableLength == 0) {
		logWarn("%! could not map SDT at phys %h, could not get table length",
				"acpi", tableLocation);
		return 0;
	}

	if (tableLength > G_SDT_MAXIMUM_BYTES) {
		logWarn("%! SDT at %h was skipped due to illegal length (%h)", "acpi",
				tableLocation, tableLength);
		return 0;
	}

	// Down/upalign physical range
	g_physical_address physStart = G_PAGE_ALIGN_DOWN(tableLocation);
	g_physical_address physEnd = G_PAGE_ALIGN_UP(tableLocation + tableLength);

	// Calculate offset of header within first page
	uint32_t mappingOffset = tableLocation - physStart;

	// Calculate amount of physical pages and allocate virtual range
	uint32_t pages = (physEnd - physStart) / G_PAGE_SIZE;
	g_virtual_address virtualBase = addressRangePoolAllocate(
			memoryVirtualRangePool, pages);

	// Could not find a virtual range of that size
	if (!virtualBase) {
		logWarn(
				"%! could not find a free virtual range to map an SDT of size %i pages",
				"acpi", pages);
		return 0;
	}

	// Map the pages
	for (g_virtual_address off = 0; off < (physEnd - physStart); off +=
			G_PAGE_SIZE)
			{
		pagingMapPage(virtualBase + off, physStart + off,
				DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	// Get the header pointer
	g_acpi_table_header* header = (g_acpi_table_header*) (virtualBase
			+ mappingOffset);

	// Validate the table
	if (!acpiValidateSDT(header)) {
		logWarn("%! descriptor table was not valid", "acpi");
		return 0;
	}

	// Now return the tables header
	return header;
}

uint32_t acpiGetRSDTentryCount() {

	uint32_t entryBytes = acpiRoot->length - sizeof(g_acpi_table_header);
	if (acpiRootIsXsdt) {
		return entryBytes / 8;
	} else {
		return entryBytes / 4;
	}
}

g_physical_address acpiGetRSDTentry(uint32_t index) {

	g_virtual_address startOfEntries = ((g_virtual_address) acpiRoot)
			+ sizeof(g_acpi_table_header);

	if (acpiRootIsXsdt) {
		return ((uint64_t*) startOfEntries)[index];
	} else {
		return ((uint32_t*) startOfEntries)[index];
	}

	return 0;
}

bool acpiEntryHasSignature(g_acpi_entry* entry, const char* signature) {
	// SDT headers always have a signature with length 4
	if (stringLength(signature) != 4) {
		return false;
	}

	// Check if the headers signature is equal to the wanted signature
	for (int i = 0; i < 4; i++) {
		if (signature[i] != entry->header->signature[i]) {
			return false;
		}
	}

	return true;
}
