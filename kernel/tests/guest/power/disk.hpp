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

#ifndef GHOST_TEST_POWER_DISK
#define GHOST_TEST_POWER_DISK

#include <cstdint>
#include <vector>

/**
 * Creates an empty 16 MiB FAT16 volume for persistence checks across power
 * transitions. Two FATs and a fixed root directory leave 8,167 data clusters.
 */
inline std::vector<uint8_t> powerTestDiskImage()
{
    std::vector<uint8_t> bytes(32768 * 512);
    auto put16 = [&bytes](unsigned offset, uint16_t value)
    {
        bytes[offset] = value;
        bytes[offset + 1] = value >> 8;
    };

    bytes[0] = 0xeb;
    bytes[1] = 0x3c;
    bytes[2] = 0x90;
    put16(11, 512);
    bytes[13] = 4;
    put16(14, 1);
    bytes[16] = 2;
    put16(17, 512);
    put16(19, 32768);
    bytes[21] = 0xf8;
    put16(22, 32);
    put16(24, 32);
    put16(26, 64);
    bytes[510] = 0x55;
    bytes[511] = 0xaa;
    for(unsigned copy = 0; copy < 2; ++copy)
    {
        auto offset = (1 + copy * 32) * 512;
        put16(offset, 0xfff8);
        put16(offset + 2, 0xffff);
    }

    return bytes;
}

#endif
