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

#include "constants.hpp"

inline uint8_t fatShortNameChecksum(const uint8_t* raw)
{
	uint8_t checksum = 0;
	for(unsigned i = 0; i < G_FAT_SHORT_NAME_SIZE; ++i)
		checksum = uint8_t((checksum >> 1) | (checksum << (G_FAT_BITS_PER_BYTE - 1))) + raw[i];
	return checksum;
}

inline uint16_t fatRead16(const uint8_t* bytes)
{
	return uint16_t(bytes[0]) | (uint16_t(bytes[1]) << G_FAT_BITS_PER_BYTE);
}

inline uint32_t fatRead32(const uint8_t* bytes)
{
	return uint32_t(fatRead16(bytes)) | (uint32_t(fatRead16(bytes + sizeof(uint16_t))) << (sizeof(uint16_t) * G_FAT_BITS_PER_BYTE));
}

inline void fatWrite16(uint8_t* bytes, uint16_t value)
{
	bytes[0] = value;
	bytes[1] = value >> G_FAT_BITS_PER_BYTE;
}

inline void fatWrite32(uint8_t* bytes, uint32_t value)
{
	fatWrite16(bytes, value);
	fatWrite16(bytes + sizeof(uint16_t), value >> (sizeof(uint16_t) * G_FAT_BITS_PER_BYTE));
}

#endif
