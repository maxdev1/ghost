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

#include <algorithm>
#include "fatdriver.hpp"

bool fatFileRead(g_fat_mount* mount, const g_fat_node& node, uint64_t offset, void* target, size_t length, size_t* read)
{
	*read = 0;
	if(node.directory)
		return false;
	if(offset >= node.size || !length)
		return true;
	if(length > node.size - offset)
		length = node.size - offset;

	auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	auto chain = fatChainGet(mount, node.firstCluster);
	if(!chain || chain->count < (uint64_t(node.size) + clusterBytes - 1) / clusterBytes)
		return false;

	auto bytes = static_cast<uint8_t*>(target);
	auto remaining = length;
	auto maximum =
			uint64_t(std::min(mount->device.info.maxTransferSectors, mount->device.buffer.size / G_BLOCK_SECTOR_SIZE)) *
			G_BLOCK_SECTOR_SIZE;
	if(!maximum)
		return false;
	while(remaining)
	{
		uint32_t cluster;
		uint64_t count;
		auto skip = offset % clusterBytes;
		auto limit = std::min(uint64_t(remaining), maximum - offset % G_BLOCK_SECTOR_SIZE);
		if(!fatChainGetRun(mount, chain, offset, limit, &cluster, &count))
			return false;
		if(!fatVolumeRead(&mount->device, mount->volume, fatClusterOffset(mount->volume, cluster) + skip, bytes, count))
			return false;

		bytes += count;
		offset += count;
		remaining -= count;
	}
	*read = length;
	return true;
}
