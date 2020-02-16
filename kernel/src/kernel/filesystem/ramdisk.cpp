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

#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/kernel.hpp"

#include "shared/utils/string.hpp"
#include "shared/memory/memory.hpp"
#include "kernel/memory/heap.hpp"

g_ramdisk* ramdiskMain = 0;

void ramdiskLoadFromModule(g_multiboot_module* module)
{
	if(ramdiskMain)
		kernelPanic("%! tried to initialize ramdisk multiple times", "kern");

	int pages = G_PAGE_ALIGN_UP(module->moduleEnd - module->moduleStart) / G_PAGE_SIZE;

	g_virtual_address newLocation = addressRangePoolAllocate(memoryVirtualRangePool, pages);
	if(newLocation == 0)
		kernelPanic("%! not enough virtual space for ramdisk remapping (%x required)", "kern", module->moduleEnd - module->moduleStart);

	for(int i = 0; i < pages; i++)
	{
		g_virtual_address virt = newLocation + i * G_PAGE_SIZE;
		g_physical_address phys = pagingVirtualToPhysical(module->moduleStart + i * G_PAGE_SIZE);
		pagingMapPage(virt, phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}
	module->moduleEnd = newLocation + (module->moduleEnd - module->moduleStart);
	module->moduleStart = newLocation;

	ramdiskMain = (g_ramdisk*) heapAllocate(sizeof(g_ramdisk));
	ramdiskParseContents(module);
	logInfo("%! module loaded: %i MB", "ramdisk", (module->moduleEnd - module->moduleStart) / 1024 / 1024);
	logDebug("%! relocated to kernel space: %h -> %h", "ramdisk", module->moduleStart, G_PAGE_ALIGN_UP(module->moduleEnd));
}

void ramdiskParseContents(g_multiboot_module* module)
{
	uint8_t* data = (uint8_t*) module->moduleStart;
	g_address dataEnd = module->moduleEnd;

	ramdiskMain->root = new g_ramdisk_entry;
	ramdiskMain->root->id = 0;
	ramdiskMain->firstEntry = 0;
	ramdiskMain->nextUnusedId = 0;

	uint32_t pos = 0;
	g_ramdisk_entry* currentHeader = 0;
	while((g_address) (data + pos) < dataEnd)
	{
		g_ramdisk_entry* entry = new g_ramdisk_entry;
		entry->next = 0;

		if(currentHeader == 0)
		{
			ramdiskMain->firstEntry = entry;
			currentHeader = ramdiskMain->firstEntry;
		} else
		{
			currentHeader->next = entry;
			currentHeader = entry;
		}

		// Type (file/folder)
		uint8_t* typeptr = (uint8_t*) (data + pos);
		entry->type = static_cast<g_ramdisk_entry_type>(*typeptr);
		pos++;

		// ID
		uint32_t* idptr = (uint32_t*) (data + pos);
		entry->id = *idptr;
		pos += 4;

		// Parent ID
		uint32_t* parentidptr = (uint32_t*) (data + pos);
		entry->parentid = *parentidptr;
		pos += 4;

		// Name
		uint32_t* namelengthptr = (uint32_t*) (data + pos);
		uint32_t namelength = *namelengthptr;
		pos += 4;

		uint8_t* nameptr = (uint8_t*) (data + pos);
		entry->name = new char[namelength + 1];
		memoryCopy(entry->name, nameptr, namelength);
		entry->name[namelength] = 0;
		pos += namelength;

		// If its a file, load content into buffer
		entry->dataOnRamdisk = true;
		if(entry->type == G_RAMDISK_ENTRY_TYPE_FILE)
		{
			// Data length
			uint32_t* datalengthptr = (uint32_t*) (data + pos);
			entry->dataSize = *datalengthptr;
			pos += 4;

			// Copy data
			entry->data = (uint8_t*) (data + pos);
			pos += entry->dataSize;
		} else
		{
			entry->dataSize = 0;
			entry->data = 0;
		}

		// start with unused ids after the last one
		if(entry->id > ramdiskMain->nextUnusedId)
		{
			ramdiskMain->nextUnusedId = entry->id + 1;
		}
	}
}

g_ramdisk_entry* ramdiskFindChild(g_ramdisk_entry* parent, const char* childName)
{

	g_ramdisk_entry* current = ramdiskMain->firstEntry;
	while(current)
	{
		if(current->parentid == parent->id && stringEquals(current->name, childName))
			return current;

		current = current->next;
	}

	return 0;
}

g_ramdisk_entry* ramdiskFindById(g_ramdisk_id id)
{
	g_ramdisk_entry* foundNode = 0;

	if(id == 0)
	{
		return ramdiskMain->root;
	}

	g_ramdisk_entry* currentNode = ramdiskMain->firstEntry;
	while(currentNode != 0)
	{
		if(currentNode->id == id)
		{
			foundNode = currentNode;
			break;
		}

		currentNode = currentNode->next;
	}

	return foundNode;
}

g_ramdisk_entry* ramdiskFindAbsolute(const char* path)
{

	return ramdiskFindRelative(ramdiskMain->root, path);
}

g_ramdisk_entry* ramdiskFindRelative(g_ramdisk_entry* node, const char* path)
{
	char buf[G_RAMDISK_MAXIMUM_PATH_LENGTH];
	uint32_t pathLen = stringLength(path);
	memoryCopy(buf, path, pathLen);
	buf[pathLen] = 0;

	g_ramdisk_entry* currentNode = node;
	while(stringLength(buf) > 0)
	{
		int slashPos = stringIndexOf(buf, '/');
		if(slashPos == -1)
		{
			currentNode = ramdiskFindChild(currentNode, buf);
			break;
		}

		if(slashPos > 0)
		{
			char childpath[G_RAMDISK_MAXIMUM_PATH_LENGTH];
			memoryCopy(childpath, buf, slashPos);
			childpath[slashPos] = 0;

			currentNode = ramdiskFindChild(currentNode, childpath);
		}

		uint32_t len = stringLength(buf) - (slashPos + 1);
		memoryCopy(buf, &buf[slashPos + 1], len);
		buf[len] = 0;
	}
	return currentNode;
}

uint32_t ramdiskGetChildCount(g_ramdisk_id id)
{
	uint32_t count = 0;

	g_ramdisk_entry* currentNode = ramdiskMain->firstEntry;
	while(currentNode != 0)
	{
		if(currentNode->parentid == id)
		{
			++count;
		}
		currentNode = currentNode->next;
	}

	return count;
}

g_ramdisk_entry* ramdiskGetChildAt(g_ramdisk_id id, uint32_t index)
{

	uint32_t pos = 0;

	g_ramdisk_entry* currentNode = ramdiskMain->firstEntry;
	while(currentNode)
	{
		if(currentNode->parentid == id)
		{
			if(pos == index)
				return currentNode;
			++pos;
		}
		currentNode = currentNode->next;
	}

	return 0;
}

g_ramdisk_entry* ramdiskGetRoot()
{
	return ramdiskMain->root;
}

g_ramdisk_entry* ramdiskCreateFile(g_ramdisk_entry* parent, const char* filename)
{
	g_ramdisk_entry* entry = (g_ramdisk_entry*) heapAllocate(sizeof(g_ramdisk_entry));
	entry->next = ramdiskMain->firstEntry;
	ramdiskMain->firstEntry = entry;

	int namelen = stringLength(filename);
	entry->name = (char*) heapAllocate(sizeof(char) * (namelen + 1));
	stringCopy(entry->name, filename);

	entry->type = G_RAMDISK_ENTRY_TYPE_FILE;
	entry->id = ramdiskMain->nextUnusedId++;
	entry->parentid = parent->id;

	entry->data = nullptr;
	entry->dataSize = 0;
	entry->dataOnRamdisk = false;
	entry->notOnRdBufferLength = 0;

	return entry;
}
