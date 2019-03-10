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

#include "kernel/system/acpi/rsdp.hpp"
#include "shared/system/bios_data_area.hpp"
#include "shared/logger/logger.hpp"

g_rsdp_descriptor* rsdpFindInRange(uint32_t start, uint32_t end)
{

	uint32_t pos = start;
	while(pos < end)
	{

		uint64_t val = *((uint64_t*) pos);
		if(val == G_RSDPTR_MAGIC)
		{
			return (g_rsdp_descriptor*) pos;
		}

		// RSDP is always at 16 byte boundary
		pos += 16;
	}

	return 0;

}

g_rsdp_descriptor* rsdpFindInEBDA()
{

	uint32_t ebdaAddress = biosDataArea->ebdaShiftedAddr << 4;

	// If the EBDA address is above 1MiB, we can assume its not valid.
	// Also we want to scan the first KiB, so we check it like this:
	if(ebdaAddress + 0x400 >= 0x100000)
	{
		logWarn("%! address of EBDA was at %h (not valid)", "rsdplookup", ebdaAddress);
		return 0;
	}

	// Scan first kilobyte of the EBDA for RDSP
	return rsdpFindInRange(ebdaAddress, ebdaAddress + 0x400);
}

bool rsdpValidate(g_rsdp_descriptor* rsdp)
{

	uint8_t baseCheckResult = 0;
	uint8_t* rsdpBytes = reinterpret_cast<uint8_t*>(rsdp);
	for(uint16_t i = 0; i < sizeof(g_rsdp_descriptor); i++)
	{
		baseCheckResult += rsdpBytes[i];
	}

	// Not zero checksum? Illegal RSDP
	if(baseCheckResult != 0)
	{
		return false;
	}

	logDebug("%! base part of RSDP is valid", "rsdplookup");

	// If we ACPI 2.0 (= revision 1) or higher we need to also validate the
	// additional fields of the RSDP separately:
	if(rsdp->revision > 0)
	{
		uint8_t extendedCheckResult = 0;
		for(uint16_t i = sizeof(g_rsdp_descriptor); i < sizeof(g_rsdp_descriptor_20); i++)
		{
			extendedCheckResult += rsdpBytes[i];
		}

		if(extendedCheckResult != 0)
		{
			return false;
		}

		logDebug("%! extended part of RSDP is valid", "rsdplookup");
	}

	// All valid
	return true;
}

g_rsdp_descriptor* rsdpFind()
{

	g_rsdp_descriptor* rsdp = 0;

	// Look in EBDA
	rsdp = rsdpFindInEBDA();
	if(rsdp == 0)
	{
		// Look in the area just below 1 MiB
		rsdp = rsdpFindInRange(0x000E0000, 0x000FFFFF);
	}

	// Check RSDP
	if(rsdp && rsdpValidate(rsdp))
	{
		logDebugn("%! found valid pointer: oemId: '", "rsdplookup");
		for(uint16_t i = 0; i < 6; i++)
		{
			logDebugn("%c", rsdp->oemId[i]);
		}

		logDebugn("', acpiVersion: ");

		if(rsdp->revision == 0)
		{
			logDebugn("1.0");
		} else if(rsdp->revision == 1)
		{
			logDebugn("2.0");
		} else if(rsdp->revision == 2)
		{
			logDebugn("3.0");
		} else
		{
			logDebugn("? (revision %i)", rsdp->revision);
		}

		logDebug("");
		return rsdp;
	}

	return 0;
}

