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

#ifndef SYSTEM_ACPI_ACPI
#define SYSTEM_ACPI_ACPI

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "ghost/types.h"
#include <system/acpi/acpi_table_header.hpp>
#include <system/acpi/acpi_entry.hpp>
#include <system/acpi/RSDP.hpp>

// Set a maximum for SDTs to avoid problems with possible junk tables
#define	G_SDT_MAXIMUM_BYTES		0x10000

/**
 *
 */
class g_acpi {
private:

	/**
	 * Prepares the root SDT (RSTD or XSDT) by mapping the required
	 * memory into the virtual address space.
	 */
	static void prepareRootSDT(g_rsdp_descriptor* rsdp);

	/**
	 * Temporarily maps the SDT at the given location to the virtual address space,
	 * retrieves its length and then unmaps it again.
	 *
	 * @return the length of the table or 0 if failed
	 */
	static uint32_t getLengthOfUnmappedSDT(g_physical_address tableLocation);

	/**
	 * Maps the table at the given physical address into the virtual space.
	 * First uses the {getLengthOfUnmappedSDT} to get the length of the table
	 * and then maps the required size in the address space.
	 *
	 * @return header of the mapped table or 0
	 */
	static g_acpi_table_header* mapSDT(g_physical_address tableLocation);

	/**
	 * Validates the whole system descriptor table with the given header
	 */
	static bool validateSDT(g_acpi_table_header* header);

	/**
	 * Returns the number of entries that the RSDT with the given header has
	 */
	static uint32_t getRSDTentryCount();

	/**
	 * Returns the RSDT entry at the given index.
	 */
	static g_physical_address getRSDTentry(uint32_t index);

public:

	/**
	 *
	 */
	static g_acpi_entry* getEntryWithSignature(const char* signature);

	/**
	 *
	 */
	static g_acpi_entry* getEntries();

	/**
	 *
	 */
	static bool hasEntries();

	/**
	 *
	 */
	static void gatherInformation();

};

#endif
