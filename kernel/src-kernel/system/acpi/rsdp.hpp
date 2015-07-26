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

#ifndef SYSTEM_ACPI_RSDP
#define SYSTEM_ACPI_RSDP

#include "ghost/stdint.h"

// "RSD PTR ", vice-versa (little endian)
#define G_RSDPTR_MAGIC	0x2052545020445352

/**
 *
 */
struct g_rsdp_descriptor {
	char signature[8];
	uint8_t checksum;
	char oemId[6];
	uint8_t revision;
	uint32_t rsdtAddress;
}__attribute__((packed));

/**
 *
 */
struct g_rsdp_descriptor_20 {
	g_rsdp_descriptor descriptor;

	uint32_t length;
	uint64_t xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
}__attribute__((packed));

#endif
