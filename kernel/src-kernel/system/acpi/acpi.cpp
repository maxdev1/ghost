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

#include <system/acpi/acpi.hpp>
#include <system/acpi/rsdp_lookup_util.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>
#include <utils/string.hpp>
#include <memory/address_space.hpp>
#include <system/acpi/MADT.hpp>
#include "ghost/types.h"

/**
 * Remembers the root header and whether its an XSDT
 */
static g_acpi_table_header* rootHeader = 0;
static bool rootIsXSDT = false;

/**
 * List of all tables in the root
 */
static g_acpi_entry* first = 0;

/**
 *
 */
g_acpi_entry* g_acpi::getEntries() {
	return first;
}

/**
 * 
 */
bool g_acpi::hasEntries() {
	return first != 0;
}

/**
 *
 */
g_acpi_entry* g_acpi::getEntryWithSignature(const char* signature) {
	g_acpi_entry* cur = first;
	while (cur) {
		if (cur->hasSignature(signature)) {
			return cur;
		}
		cur = cur->next;
	}
	return 0;
}

/**
 *
 */
void g_acpi::gatherInformation() {

	g_log_debug("%! gathering information", "acpi");

	// Find root table pointer
	g_rsdp_descriptor* rsdp = g_rsdp_lookup_util::findRSDP();
	if (rsdp) {

		// Prepare the root table
		g_acpi::prepareRootSDT(rsdp);

		// Go through the existing root entries
		for (uint32_t i = 0; i < getRSDTentryCount(); i++) {
			g_physical_address entry = getRSDTentry(i);
			if (entry != 0) {
				g_acpi_table_header* sdt = mapSDT(entry);

				// Could not be mapped? Skip
				if (sdt == 0) {
					continue;
				}

				// Create the entry
				g_acpi_entry* entry = new g_acpi_entry();
				entry->header = sdt;
				entry->next = first;
				first = entry;
			}
		}

	} else {
		g_log_info("%! could not find RSDP", "acpi");
	}
}

/**
 * 
 */
void g_acpi::prepareRootSDT(g_rsdp_descriptor* rsdp) {

	g_physical_address rootTableLocation = 0;

	rootIsXSDT = false;

	// If ACPI 2.0 or higher, try to use the XSDT
	if (rsdp->revision > 0) {

		g_rsdp_descriptor_20* rsdp20 = (g_rsdp_descriptor_20*) rsdp;
		if (rsdp20->xsdtAddress != 0) {
#if _ARCH_X86_64_
			rootTableLocation = rsdp20->xsdtAddress;
			g_log_debug("%! found XSDT in 64bit range", "acpi");
			rootIsXSDT = true;
#elif _ARCH_X86_
			if (rsdp20->xsdtAddress < 0xFFFFFFFF) {
				rootIsXSDT = true;
				g_log_debug("%! found XSDT in 32bit range", "acpi");
				rootTableLocation = rsdp20->xsdtAddress;
			} else {
				rootIsXSDT = false;
				g_log_debug("%! found XSDT, but range too high for 32bits, attempting to use RSDT", "acpi");
			}
#endif
		}
	}

	// No XSDT? Use RSDT
	if (!rootIsXSDT) {
		g_log_debug("%! no XSDT; using RSDT", "acpi");
		rootTableLocation = rsdp->rsdtAddress;
	}

	// No header? Failed
	if (rootTableLocation == 0) {
		g_log_warn("%! RSDP did not contain a valid RSDT/XSDT address", "acpi");
		return;
	}

	g_log_debug("%! root table starts at phys %h", "acpi", rootTableLocation);

	// Map table in virtual address space
	g_acpi_table_header* header = mapSDT(rootTableLocation);
	if (header == 0) {
		g_log_warn("%! could not map root system descriptor table", "acpi");
		return;
	}

	rootHeader = header;
}

/**
 * 
 */
bool g_acpi::validateSDT(g_acpi_table_header* header) {

	uint8_t sum = 0;
	uint8_t* tableBytes = reinterpret_cast<uint8_t*>(header);

	for (uint32_t i = 0; i < header->length; i++) {
		sum += tableBytes[i];
	}

	return sum == 0;
}

/**
 * 
 */
uint32_t g_acpi::getLengthOfUnmappedSDT(g_physical_address tableLocation) {

	// Align the location down, we will allocate 2 pages to make sure the
	// header is within the range
	g_physical_address physStart = G_PAGE_ALIGN_DOWN(tableLocation);
	g_virtual_address virtualBase = g_kernel::virtual_range_pool->allocate(2);

	if (!g_address_space::map(virtualBase, physStart, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS)) {
		g_log_warn("%! could not create virtual mapping (1) for SDT %h", "acpi", tableLocation);
		return 0;
	}
	if (!g_address_space::map(virtualBase + G_PAGE_SIZE, physStart + G_PAGE_SIZE, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS)) {
		g_log_warn("%! could not create virtual mapping (2) for SDT %h", "acpi", tableLocation);
		return 0;
	}

	// Calculate the offset of the header within the table
	uint32_t mappingOffset = tableLocation - physStart;

	// Take length from the header
	g_acpi_table_header* header = (g_acpi_table_header*) (virtualBase + mappingOffset);
	uint32_t length = header->length;

	// Unmap the two mapped pages
	g_address_space::unmap(virtualBase);
	g_address_space::unmap(virtualBase + G_PAGE_SIZE);
	g_kernel::virtual_range_pool->free(virtualBase);

	return length;
}

/**
 * 
 */
g_acpi_table_header* g_acpi::mapSDT(g_physical_address tableLocation) {

	// Retrieve the tables length
	uint32_t tableLength = getLengthOfUnmappedSDT(tableLocation);
	if (tableLength == 0) {
		g_log_warn("%! could not map SDT at phys %h, could not get table length", "acpi", tableLocation);
		return 0;
	}

	// Does the length make sense?
	if (tableLength > G_SDT_MAXIMUM_BYTES) {
		g_log_warn("%! SDT at %h was skipped due to illegal length (%h)", "acpi", tableLocation, tableLength);
		return 0;
	}

	// Down/upalign physical range
	g_physical_address physStart = G_PAGE_ALIGN_DOWN(tableLocation);
	g_physical_address physEnd = G_PAGE_ALIGN_UP(tableLocation + tableLength);

	// Calculate offset of header within first page
	uint32_t mappingOffset = tableLocation - physStart;

	// Calculate amount of physical pages and allocate virtual range
	uint32_t pages = (physEnd - physStart) / G_PAGE_SIZE;
	g_virtual_address virtualBase = g_kernel::virtual_range_pool->allocate(pages);

	// Could not find a virtual range of that size
	if (virtualBase == 0) {
		g_log_warn("%! could not find a free virtual range to map an SDT of size %i pages", "acpi", pages);
		return 0;
	}

	// Map the pages
	for (g_virtual_address off = 0; off < (physEnd - physStart); off += G_PAGE_SIZE) {
		g_address_space::map(virtualBase + off, physStart + off, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	// Get the header pointer
	g_acpi_table_header* header = (g_acpi_table_header*) (virtualBase + mappingOffset);

	// Validate the table
	if (!validateSDT(header)) {
		g_log_warn("%! descriptor table was not valid", "acpi");
		return 0;
	}

	// Now return the tables header
	return header;
}

/**
 * 
 */
uint32_t g_acpi::getRSDTentryCount() {

	uint32_t entryBytes = rootHeader->length - sizeof(g_acpi_table_header);
	if (rootIsXSDT) {
		return entryBytes / 8;
	} else {
		return entryBytes / 4;
	}
}

/**
 * 
 */
g_physical_address g_acpi::getRSDTentry(uint32_t index) {

	g_virtual_address startOfEntries = ((g_virtual_address) rootHeader) + sizeof(g_acpi_table_header);

	if (rootIsXSDT) {
		return ((uint64_t*) startOfEntries)[index];
	} else {
		return ((uint32_t*) startOfEntries)[index];
	}

	return 0;
}
