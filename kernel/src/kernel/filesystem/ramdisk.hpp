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

#ifndef __KERNEL_RAMDISK__
#define __KERNEL_RAMDISK__

#include "ghost/stdint.h"
#include "ghost/ramdisk.h"

#include "kernel/filesystem/ramdisk_entry.hpp"
#include "shared/multiboot/multiboot.hpp"

struct g_ramdisk
{
	g_ramdisk_entry* firstEntry;
	g_ramdisk_entry* root;
	uint32_t nextUnusedId = 0;
};

extern g_ramdisk* ramdiskMain;

/**
 * Creates a ramdisk instance for a multiboot module. The contents of the
 * ramdisk are relocated into kernel memory.
 *
 * @param module source multiboot module
 * @return a ramdisk instance
 */
void ramdiskLoadFromModule(g_multiboot_module* module);

void ramdiskParseContents(g_multiboot_module* module);

/**
 * Searches in the folder parent for a file/folder with the given name
 *
 * @param parent the parent folder to search through
 * @param childName the name of the child to find
 * @return the entry if found, else 0
 */
g_ramdisk_entry* ramdiskFindChild(g_ramdisk_entry* node, const char* name);

/**
 * Searches for the entry at the given absolute path
 *
 * @param name	name of the entry to find
 * @return the entry if found, else 0
 */
g_ramdisk_entry* ramdiskFindAbsolute(const char* name);

/**
 * Searches for a child with of "node" with the given relative path
 *
 * @param node	the parent node
 * @param path	the relative child path
 * @return the entry if found, else 0
 */
g_ramdisk_entry* ramdiskFindRelative(g_ramdisk_entry* node, const char* path);

/**
 * Returns the entry with the given "id"
 *
 * @param name the name
 * @return the entry if it exists, else 0
 */
g_ramdisk_entry* ramdiskFindById(g_ramdisk_id id);

/**
 * Returns the number of children that the entry with "id" has
 *
 * @param id	the entries id
 * @return the number of children
 */
uint32_t ramdiskGetChildCount(g_ramdisk_id id);

/**
 * Returns the child at "index" of the node "id"
 *
 * @param id	the parent entries id
 * @param index	the index of the child
 * @return the entry if found, else 0
 */
g_ramdisk_entry* ramdiskGetChildAt(g_ramdisk_id id, uint32_t index);

/**
 * Returns the root.
 */
g_ramdisk_entry* ramdiskGetRoot();

/**
 * Creates a new child node on the ramdisk.
 */
g_ramdisk_entry* ramdiskCreateFile(g_ramdisk_entry* parent, const char* filename);

#endif
