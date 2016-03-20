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

#include <kernel.hpp>

#include <ramdisk/ramdisk.hpp>
#include <utils/string.hpp>
#include <logger/logger.hpp>

/**
 * 
 */
g_ramdisk::g_ramdisk() {
	root = 0;
	firstHeader = 0;
}

/**
 * 
 */
g_ramdisk_entry* g_ramdisk::load(g_multiboot_module* module) {
	// Get the ramdisk location and its end from the multiboot info
	uint8_t* ramdisk = (uint8_t*) module->moduleStart;
	uint32_t ramdiskEnd = module->moduleEnd;

	// Create a root RamdiskEntry
	root = new g_ramdisk_entry;
	root->id = 0;

	// Initialize the header storage location
	firstHeader = 0;

	// Create the position
	uint32_t ramdiskPosition = 0;

	// Iterate through the ramdisk
	g_ramdisk_entry* currentHeader = 0;
	while ((uint32_t) (ramdisk + ramdiskPosition) < ramdiskEnd) {
		g_ramdisk_entry* header = new g_ramdisk_entry;
		header->next = 0;

		if (currentHeader == 0) {
			firstHeader = header;
			currentHeader = firstHeader;
		} else {
			currentHeader->next = header;
			currentHeader = header;
		}

		// Type (file/folder)
		uint8_t* typeptr = (uint8_t*) (ramdisk + ramdiskPosition);
		header->type = static_cast<g_ramdisk_entry_type>(*typeptr);
		ramdiskPosition++;

		// ID
		uint32_t* idptr = (uint32_t*) (ramdisk + ramdiskPosition);
		header->id = *idptr;
		ramdiskPosition += 4;

		// parent ID
		uint32_t* parentidptr = (uint32_t*) (ramdisk + ramdiskPosition);
		header->parentid = *parentidptr;
		ramdiskPosition += 4;

		// Name
		uint32_t* namelengthptr = (uint32_t*) (ramdisk + ramdiskPosition);
		uint32_t namelength = *namelengthptr;
		ramdiskPosition += 4;

		uint8_t* nameptr = (uint8_t*) (ramdisk + ramdiskPosition);
		header->name = new char[namelength + 1];
		g_memory::copy(header->name, nameptr, namelength);
		header->name[namelength] = 0;
		ramdiskPosition += namelength;

		// If its a file, load rest
		header->data_on_ramdisk = true;
		if (header->type == G_RAMDISK_ENTRY_TYPE_FILE) {
			// Data length
			uint32_t* datalengthptr = (uint32_t*) (ramdisk + ramdiskPosition);
			header->datalength = *datalengthptr;
			ramdiskPosition += 4;

			// Copy data
			header->data = (uint8_t*) (ramdisk + ramdiskPosition);
			ramdiskPosition += header->datalength;
		} else {
			header->datalength = 0;
			header->data = 0;
		}

		// start with unused ids after the last one
		if (header->id > next_unused_id) {
			next_unused_id = header->id + 1;
		}
	}

	return this->root;
}

/**
 * 
 */
g_ramdisk_entry* g_ramdisk::findChild(g_ramdisk_entry* parent, const char* childName) {

	g_ramdisk_entry* current = firstHeader;
	do {
		if (current->parentid == parent->id && g_string::equals(current->name, childName)) {
			return current;
		}
	} while ((current = current->next) != 0);

	return 0;
}

/**
 * 
 */
g_ramdisk_entry* g_ramdisk::findById(uint32_t id) {
	g_ramdisk_entry* foundNode = 0;

	if (id == 0) {
		return root;
	}

	g_ramdisk_entry* currentNode = firstHeader;
	while (currentNode != 0) {
		if (currentNode->id == id) {
			foundNode = currentNode;
			break;
		}

		currentNode = currentNode->next;
	}

	return foundNode;
}

/**
 * 
 */
g_ramdisk_entry* g_ramdisk::findAbsolute(const char* path) {

	return findRelative(root, path);
}

/**
 * Searches for the entry at the given the relative path to the given node.
 */
g_ramdisk_entry* g_ramdisk::findRelative(g_ramdisk_entry* node, const char* path) {
	char buf[G_RAMDISK_MAXIMUM_PATH_LENGTH];
	uint32_t pathLen = g_string::length(path);
	g_memory::copy(buf, path, pathLen);
	buf[pathLen] = 0;

	g_ramdisk_entry* currentNode = node;
	while (g_string::length(buf) > 0) {
		int slashIndex = g_string::indexOf(buf, '/');
		if (slashIndex >= 0) {
			// Set current node to next layer
			if (slashIndex > 0) {
				char childpath[G_RAMDISK_MAXIMUM_PATH_LENGTH];
				g_memory::copy(childpath, buf, slashIndex);
				childpath[slashIndex] = 0;

				currentNode = findChild(currentNode, childpath);
			}

			// Cut off layer
			uint32_t len = g_string::length(buf) - (slashIndex + 1);
			g_memory::copy(buf, &buf[slashIndex + 1], len);
			buf[len] = 0;
		} else {
			// Reached the last node, find the child
			currentNode = findChild(currentNode, buf);
			break;
		}
	}
	return currentNode;
}

/**
 * 
 */
uint32_t g_ramdisk::getChildCount(uint32_t id) {

	uint32_t count = 0;

	g_ramdisk_entry* currentNode = firstHeader;
	while (currentNode != 0) {
		if (currentNode->parentid == id) {
			++count;
		}
		currentNode = currentNode->next;
	}

	return count;
}

/**
 * 
 */
g_ramdisk_entry* g_ramdisk::getChildAt(uint32_t id, uint32_t index) {

	uint32_t pos = 0;

	g_ramdisk_entry* currentNode = firstHeader;
	while (currentNode != 0) {
		if (currentNode->parentid == id) {
			if (pos == index) {
				return currentNode;
			}

			++pos;
		}
		currentNode = currentNode->next;
	}

	return 0;
}

/**
 *
 */
g_ramdisk_entry* g_ramdisk::getRoot() const {
	return root;
}

/**
 *
 */
g_ramdisk_entry* g_ramdisk::createChild(g_ramdisk_entry* parent, char* filename) {

	g_ramdisk_entry* new_node = new g_ramdisk_entry();
	new_node->next = firstHeader;
	firstHeader = new_node;

	// copy name
	int namelen = g_string::length(filename);
	new_node->name = new char[namelen + 1];
	g_string::copy(new_node->name, filename);

	new_node->type = G_RAMDISK_ENTRY_TYPE_FILE;
	new_node->id = next_unused_id++;
	new_node->parentid = parent->id;

	// set empty buffer
	new_node->data = nullptr;
	new_node->datalength = 0;
	new_node->not_on_rd_buffer_length = 0;
	new_node->data_on_ramdisk = false;

	return new_node;
}
