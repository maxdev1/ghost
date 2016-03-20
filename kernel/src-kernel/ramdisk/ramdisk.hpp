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

#ifndef GHOST_RAMDISK_RAMDSIK
#define GHOST_RAMDISK_RAMDSIK

#include "ghost/stdint.h"
#include "ghost/ramdisk.h"
#include <ramdisk/ramdisk_entry.hpp>
#include <multiboot/multiboot.hpp>

/**
 * Ramdisk class
 */
class g_ramdisk {
private:
	g_ramdisk_entry* firstHeader;
	g_ramdisk_entry* root;
	uint32_t next_unused_id = 0;

public:
	/**
	 * Initializes the empty ramdisk. A ramdisk is never deleted, therefore
	 * there is no destructor.
	 */
	g_ramdisk();

	/**
	 * Loads the ramdisk by creating actual {RamdiskEntry} objects. Reads the "module"
	 * and interprets it as a ramdisk filesystem
	 *
	 * @param module	the ramdisk multiboot module
	 */
	g_ramdisk_entry* load(g_multiboot_module* module);

	/**
	 * Searches in the folder parent for a file/folder with the given name
	 *
	 * @param parent the parent folder to search through
	 * @param childName the name of the child to find
	 * @return the entry if found, else 0
	 */
	g_ramdisk_entry* findChild(g_ramdisk_entry* node, const char* name);

	/**
	 * Searches for the entry at the given absolute path
	 *
	 * @param name	name of the entry to find
	 * @return the entry if found, else 0
	 */
	g_ramdisk_entry* findAbsolute(const char* name);

	/**
	 * Searches for a child with of "node" with the given relative path
	 *
	 * @param node	the parent node
	 * @param path	the relative child path
	 * @return the entry if found, else 0
	 */
	g_ramdisk_entry* findRelative(g_ramdisk_entry* node, const char* path);

	/**
	 * Returns the entry with the given "id"
	 *
	 * @param name the name
	 * @return the entry if it exists, else 0
	 */
	g_ramdisk_entry* findById(uint32_t id);

	/**
	 * Returns the number of children that the entry with "id" has
	 *
	 * @param id	the entries id
	 * @return the number of children
	 */
	uint32_t getChildCount(uint32_t id);

	/**
	 * Returns the child at "index" of the node "id"
	 *
	 * @param id	the parent entries id
	 * @param index	the index of the child
	 * @return the entry if found, else 0
	 */
	g_ramdisk_entry* getChildAt(uint32_t id, uint32_t index);

	/**
	 * Returns the root.
	 */
	g_ramdisk_entry* getRoot() const;

	/**
	 *
	 */
	g_ramdisk_entry* createChild(g_ramdisk_entry* parent, char* filename);

};

#endif
