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

#ifndef SYSTEM_ACPI_RSDPLOOKUPUTIL
#define SYSTEM_ACPI_RSDPLOOKUPUTIL

#include "ghost/stdint.h"
#include "ghost/types.h"
#include <system/acpi/rsdp.hpp>

/**
 *
 */
class g_rsdp_lookup_util {
public:

	/**
	 * Looks for the RSDP in the range from "start" to "end".
	 *
	 * @return the RSDP, otherwise 0.
	 */
	static g_rsdp_descriptor* findRSDPinRange(uint32_t start, uint32_t end);

	/**
	 * Searches for the RSDP in the Extended BIOS Data Area.
	 *
	 * @return the RSDP, otherwise 0.
	 */
	static g_rsdp_descriptor* findRSDPinEBDA();

	/**
	 * Validates the "rsdp" by checking its revision and then using the
	 * right checksum method.
	 *
	 * @return true if the pointer is valid
	 */
	static bool validateRSDP(g_rsdp_descriptor* rsdp);

	/**
	 * Attempts to find the Root System Description Pointer
	 *
	 * @return either the RSDP or 0
	 */
	static g_rsdp_descriptor* findRSDP();

};

#endif
