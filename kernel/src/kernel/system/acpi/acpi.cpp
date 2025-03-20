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

#include "kernel/system/acpi/acpi.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"
#include "shared/utils/string.hpp"
#include "shared/memory/constants.hpp"
#include <ghost/memory/types.h>

static g_acpi_table_header* acpiRoot = nullptr;
static g_acpi_entry* acpiTables = nullptr;
static bool acpiRootIsXsdt = false;

g_acpi_entry* acpiGetEntryWithSignature(const char* signature)
{
	g_acpi_entry* cur = acpiTables;
	while(cur)
	{
		if(acpiEntryHasSignature(cur, signature))
		{
			return cur;
		}
		cur = cur->next;
	}
	return nullptr;
}

void acpiInitialize(g_physical_address rsdpPhys)
{
	auto rsdpPage = G_PAGE_ALIGN_DOWN(rsdpPhys);
	pagingMapPage(G_MEM_PHYS_TO_VIRT(rsdpPage), rsdpPage,G_PAGE_TABLE_KERNEL_DEFAULT,G_PAGE_KERNEL_DEFAULT);
	auto rsdp = (g_rsdp_descriptor*) G_MEM_PHYS_TO_VIRT(rsdpPhys);
	if(!rsdp)
		panic("%! failed to find RSDP", "acpi");

	acpiPrepareRootSDT(rsdp);

	for(uint32_t i = 0; i < acpiGetRSDTentryCount(); i++)
	{
		g_physical_address sdtPhys = acpiGetRSDTentry(i);
		if(!sdtPhys)
			continue;

		g_acpi_table_header* sdt = acpiMapSDT(sdtPhys);
		if(!sdt)
			continue;

		auto entry = new g_acpi_entry();
		entry->header = sdt;
		entry->next = acpiTables;
		acpiTables = entry;
	}
}

void acpiPrepareRootSDT(g_rsdp_descriptor* rsdp)
{
	g_physical_address rootTablePhys = 0;
	acpiRootIsXsdt = false;

	// If ACPI 2.0 or higher, try to use the XSDT
	if(rsdp->revision > 0)
	{
		auto rsdp20 = (g_rsdp_descriptor_20*) rsdp;
		if(rsdp20->xsdtAddress != 0)
		{
			rootTablePhys = rsdp20->xsdtAddress;
			logDebug("%! found XSDT in 64bit range", "acpi");
			acpiRootIsXsdt = true;
		}
	}

	// No XSDT? Use RSDT
	if(!acpiRootIsXsdt)
		rootTablePhys = rsdp->rsdtAddress;

	if(!rootTablePhys)
		panic("%! failed to find ACPI root table", "acpi");

	g_acpi_table_header* header = acpiMapSDT(rootTablePhys);
	if(!header)
		panic("%! could not map root system descriptor table", "acpi");

	acpiRoot = header;
}

bool acpiValidateSDT(g_acpi_table_header* header)
{
	uint8_t sum = 0;
	auto tableBytes = reinterpret_cast<uint8_t*>(header);

	for(uint32_t i = 0; i < header->length; i++)
		sum += tableBytes[i];

	return sum == 0;
}

uint32_t acpiGetLengthOfUnmappedSDT(g_physical_address tableLocation)
{
	g_physical_address physStart = G_PAGE_ALIGN_DOWN(tableLocation);
	g_virtual_address virtualBase = addressRangePoolAllocate(
			memoryVirtualRangePool, 2); // TODO may as well direct map to higher half

	if(!pagingMapPage(virtualBase, physStart, G_PAGE_TABLE_KERNEL_DEFAULT,
	                  G_PAGE_KERNEL_DEFAULT))
	{
		logWarn("%! could not create virtual mapping (1) for SDT %h", "acpi",
		        tableLocation);
		return 0;
	}
	if(!pagingMapPage(virtualBase + G_PAGE_SIZE, physStart + G_PAGE_SIZE,
	                  G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT))
	{
		logWarn("%! could not create virtual mapping (2) for SDT %h", "acpi",
		        tableLocation);
		return 0;
	}

	uint32_t mappingOffset = tableLocation - physStart;

	auto header = (g_acpi_table_header*) (virtualBase + mappingOffset);
	uint32_t length = header->length;

	pagingUnmapPage(virtualBase);
	pagingUnmapPage(virtualBase + G_PAGE_SIZE);
	addressRangePoolFree(memoryVirtualRangePool, virtualBase);

	return length;
}

g_acpi_table_header* acpiMapSDT(g_physical_address tableLocation)
{
	uint32_t tableLength = acpiGetLengthOfUnmappedSDT(tableLocation);
	if(tableLength == 0)
	{
		logWarn("%! could not map SDT at phys %h, could not get table length",
		        "acpi", tableLocation);
		return nullptr;
	}

	if(tableLength > G_SDT_MAXIMUM_BYTES)
	{
		logWarn("%! SDT at %h was skipped due to illegal length (%h)", "acpi",
		        tableLocation, tableLength);
		return nullptr;
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
	if(!virtualBase)
	{
		logWarn(
				"%! could not find a free virtual range to map an SDT of size %i pages",
				"acpi", pages);
		return nullptr;
	}

	// Map the pages
	for(g_virtual_address off = 0; off < (physEnd - physStart); off += G_PAGE_SIZE)
	{
		pagingMapPage(virtualBase + off, physStart + off,
		              G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
	}

	// Get the header pointer
	auto header = (g_acpi_table_header*) (virtualBase + mappingOffset);

	// Validate the table
	if(!acpiValidateSDT(header))
	{
		logWarn("%! descriptor table was not valid", "acpi");
		return nullptr;
	}

	return header;
}

uint32_t acpiGetRSDTentryCount()
{
	uint32_t entryBytes = acpiRoot->length - sizeof(g_acpi_table_header);
	if(acpiRootIsXsdt)
	{
		return entryBytes / 8;
	}
	return entryBytes / 4;
}

g_physical_address acpiGetRSDTentry(uint32_t index)
{
	g_virtual_address startOfEntries = ((g_virtual_address) acpiRoot) + sizeof(g_acpi_table_header);

	if(acpiRootIsXsdt)
	{
		return ((uint64_t*) startOfEntries)[index];
	}
	return ((uint32_t*) startOfEntries)[index];
}

bool acpiEntryHasSignature(g_acpi_entry* entry, const char* signature)
{
	// SDT headers always have a signature with length 4
	if(stringLength(signature) != 4)
	{
		return false;
	}

	// Check if the headers signature is equal to the wanted signature
	for(int i = 0; i < 4; i++)
	{
		if(signature[i] != entry->header->signature[i])
		{
			return false;
		}
	}

	return true;
}
