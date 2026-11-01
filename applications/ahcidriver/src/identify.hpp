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

#ifndef AHCIDRIVER_IDENTIFY
#define AHCIDRIVER_IDENTIFY

#include <cstddef>
#include <libahci/ata.hpp>
#include <libblockdevice/interface.hpp>

inline void ahciDecodeIdentifyString(const uint16_t* words, size_t count, char* out)
{
	for(size_t i = 0; i < count; ++i)
	{
		out[2 * i] = static_cast<char>(words[i] >> 8);
		out[2 * i + 1] = static_cast<char>(words[i]);
	}
	size_t length = count * 2;
	while(length && out[length - 1] == ' ')
		--length;
	out[length] = 0;
}

/**
 * Decodes the IDENTIFY words used by DMA/LBA48 disks with 512-byte logical
 * sectors, including 512e. Write access requires an advertised flush command.
 */
inline bool ahciParseIdentify(const uint16_t* words, g_block_device_info* out, uint8_t* flushCommand)
{
	uint16_t capabilities = words[G_ATA_IDENTIFY_CAPABILITIES];
	uint16_t commandSets = words[G_ATA_IDENTIFY_COMMAND_SETS];
	uint16_t sectorGeometry = words[G_ATA_IDENTIFY_SECTOR_GEOMETRY];
	if((words[G_ATA_IDENTIFY_CONFIGURATION] & 0x8000) ||
	   (capabilities & G_ATA_CAPABILITY_DMA_LBA) != G_ATA_CAPABILITY_DMA_LBA ||
	   (commandSets & G_ATA_COMMAND_SET_LBA48_MASK) != G_ATA_COMMAND_SET_LBA48_VALID)
		return false;

	auto capacity = words + G_ATA_IDENTIFY_LBA48_SECTORS;
	uint64_t sectors = uint64_t(capacity[0]) | (uint64_t(capacity[1]) << 16) | (uint64_t(capacity[2]) << 32) |
					   (uint64_t(capacity[3]) << 48);

	if(!sectors || sectors > G_ATA_LBA48_LIMIT)
		return false;

	auto sectorWords = words + G_ATA_IDENTIFY_LOGICAL_SECTOR_WORDS;
	if((sectorGeometry & 0xc000) == 0x4000 && (sectorGeometry & 0x1000) &&
	   (sectorWords[0] != G_BLOCK_SECTOR_SIZE / sizeof(uint16_t) || sectorWords[1] != 0))
		return false;

	*out = {};
	out->sectorCount = sectors;
	out->sectorSize = G_BLOCK_SECTOR_SIZE;
	out->maxTransferSectors = G_BLOCK_DMA_BUFFER_BYTES / G_BLOCK_SECTOR_SIZE;
	*flushCommand = 0;
	if(commandSets & G_ATA_COMMAND_SET_FLUSH_EXT)
		*flushCommand = G_ATA_COMMAND_FLUSH_CACHE_EXT;
	else if(commandSets & G_ATA_COMMAND_SET_FLUSH)
		*flushCommand = G_ATA_COMMAND_FLUSH_CACHE;

	out->flags = *flushCommand ? G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED : 0;
	ahciDecodeIdentifyString(words + G_ATA_IDENTIFY_MODEL, 20, out->model);
	ahciDecodeIdentifyString(words + G_ATA_IDENTIFY_SERIAL, 10, out->serial);
	return true;
}

#endif
