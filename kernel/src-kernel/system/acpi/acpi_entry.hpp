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

#ifndef SYSTEM_ACPI_ACPIENTRY
#define SYSTEM_ACPI_ACPIENTRY

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include <system/acpi/acpi_table_header.hpp>
#include <system/acpi/rsdp.hpp>

/**
 * Entries for a linked list of all ACPI tables
 */
class g_acpi_entry {
public:
	g_acpi_table_header* header;
	g_acpi_entry* next;

	/**
	 *
	 */
	bool hasSignature(const char* str);
};

#endif
