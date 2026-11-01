/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBAHCI_ATA
#define LIBAHCI_ATA

#include <cstdint>

constexpr uint8_t G_ATA_COMMAND_IDENTIFY = 0xec;
constexpr uint8_t G_ATA_COMMAND_READ_DMA_EXT = 0x25;
constexpr uint8_t G_ATA_COMMAND_WRITE_DMA_EXT = 0x35;
constexpr uint8_t G_ATA_COMMAND_FLUSH_CACHE = 0xe7;
constexpr uint8_t G_ATA_COMMAND_FLUSH_CACHE_EXT = 0xea;

enum g_ata_identify_word : uint16_t
{
	G_ATA_IDENTIFY_CONFIGURATION = 0,
	G_ATA_IDENTIFY_SERIAL = 10,
	G_ATA_IDENTIFY_MODEL = 27,
	G_ATA_IDENTIFY_CAPABILITIES = 49,
	G_ATA_IDENTIFY_COMMAND_SETS = 83,
	G_ATA_IDENTIFY_LBA48_SECTORS = 100,
	G_ATA_IDENTIFY_SECTOR_GEOMETRY = 106,
	G_ATA_IDENTIFY_LOGICAL_SECTOR_WORDS = 117
};

constexpr uint16_t G_ATA_CAPABILITY_DMA_LBA = 0x300;
constexpr uint16_t G_ATA_COMMAND_SET_LBA48_VALID = 0x4400;
constexpr uint16_t G_ATA_COMMAND_SET_LBA48_MASK = 0xc400;
constexpr uint16_t G_ATA_COMMAND_SET_FLUSH = 1 << 12;
constexpr uint16_t G_ATA_COMMAND_SET_FLUSH_EXT = 1 << 13;
constexpr uint64_t G_ATA_LBA48_LIMIT = uint64_t(1) << 48;

/**
 * ATA IDENTIFY payload. Word offsets select capabilities, capacity and strings
 * without relying on a compiler-specific view of the obsolete geometry fields.
 */
struct g_ata_identify_data
{
	uint16_t words[256];
};

#endif
