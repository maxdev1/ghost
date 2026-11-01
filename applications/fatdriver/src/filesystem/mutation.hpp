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

#ifndef FATDRIVER_MUTATION
#define FATDRIVER_MUTATION

#include <vector>
#include "fatdriver.hpp"

bool fatTableGet(g_fat_mount* mount, uint32_t cluster, uint32_t* value);

bool fatTableSet(g_fat_mount* mount, uint32_t cluster, uint32_t value, bool preserveChains = false);

bool fatCollectChain(g_fat_mount* mount, uint32_t first, std::vector<uint32_t>* chain);

g_fs_mutation_status fatAllocateClusters(g_fat_mount* mount, uint32_t count, std::vector<uint32_t>* allocated);

bool fatFreeClusters(g_fat_mount* mount, const std::vector<uint32_t>& chain, size_t first = 0);

bool fatInodeStore(g_fat_mount* mount, g_fat_inode* inode);

g_fs_mutation_status fatMutationAllowed(g_fat_mount* mount, const g_fat_inode* inode = nullptr);

g_fat_result fatDirectoryRawNext(g_fat_mount* mount, g_fat_directory* directory, uint64_t* offset, uint8_t* raw);

#endif
