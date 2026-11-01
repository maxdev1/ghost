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

#ifndef FATDRIVER_DISK
#define FATDRIVER_DISK

#include <cstdint>

constexpr unsigned G_FAT_DIRECTORY_ENTRY_SIZE = 32;
constexpr unsigned G_FAT_SHORT_NAME_SIZE = 11;
constexpr unsigned G_FAT_LONG_NAME_UNITS_PER_ENTRY = 13;
constexpr unsigned G_FAT_LONG_NAME_MAX_ENTRIES = 20;
constexpr uint8_t G_FAT_ATTRIBUTE_READ_ONLY = 0x01;
constexpr uint8_t G_FAT_ATTRIBUTE_VOLUME = 0x08;
constexpr uint8_t G_FAT_ATTRIBUTE_DIRECTORY = 0x10;
constexpr uint8_t G_FAT_ATTRIBUTE_ARCHIVE = 0x20;
constexpr uint8_t G_FAT_ATTRIBUTE_LONG_NAME = 0x0f;
constexpr uint8_t G_FAT_ENTRY_DELETED = 0xe5;
constexpr unsigned G_FAT_LONG_NAME_OFFSETS[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

inline uint8_t fatShortNameChecksum(const uint8_t* raw)
{
	uint8_t checksum = 0;
	for(unsigned i = 0; i < G_FAT_SHORT_NAME_SIZE; ++i)
		checksum = uint8_t((checksum >> 1) | (checksum << 7)) + raw[i];
	return checksum;
}

inline uint16_t fatRead16(const uint8_t* bytes)
{
	return uint16_t(bytes[0]) | (uint16_t(bytes[1]) << 8);
}

inline uint32_t fatRead32(const uint8_t* bytes)
{
	return uint32_t(fatRead16(bytes)) | (uint32_t(fatRead16(bytes + 2)) << 16);
}

inline void fatWrite16(uint8_t* bytes, uint16_t value)
{
	bytes[0] = value;
	bytes[1] = value >> 8;
}

inline void fatWrite32(uint8_t* bytes, uint32_t value)
{
	fatWrite16(bytes, value);
	fatWrite16(bytes + 2, value >> 16);
}

#endif
