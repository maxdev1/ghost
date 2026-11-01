/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef STORAGEMANAGER_DISK
#define STORAGEMANAGER_DISK

#include <cstdint>

constexpr uint32_t G_VOLUME_SECTOR_SIZE = 512;
constexpr uint32_t G_VOLUME_MAX_TRANSFER_SECTORS = 8;

inline uint32_t diskRead32(const uint8_t* bytes)
{
	return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) | (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}

inline uint64_t diskRead64(const uint8_t* bytes) { return diskRead32(bytes) | (uint64_t(diskRead32(bytes + 4)) << 32); }

inline void diskWrite32(uint8_t* bytes, uint32_t value)
{
	for(unsigned i = 0; i < 4; ++i)
		bytes[i] = value >> (i * 8);
}

inline bool diskHasSignature(const uint8_t* sector) { return sector[510] == 0x55 && sector[511] == 0xaa; }

/**
 * Recognize the FAT boot-sector geometry before interpreting its overlapping
 * bytes as an MBR partition table. This does not validate the filesystem.
 */
inline bool diskHasFilesystem(const uint8_t* sector)
{
	return (sector[0] == 0xeb || sector[0] == 0xe9) && sector[11] == 0 && sector[12] == 2 && sector[13] &&
		   !(sector[13] & (sector[13] - 1)) && (sector[14] || sector[15]) && (sector[16] == 1 || sector[16] == 2);
}

inline bool diskIsExtended(uint32_t type) { return type == 0x05 || type == 0x0f || type == 0x85; }

#endif
