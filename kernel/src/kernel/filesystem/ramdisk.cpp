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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/panic.hpp"
#include "kernel/utils/string.hpp"

g_ramdisk* ramdiskMain = nullptr;

void ramdiskLoadFromBootloaderFile(limine_file* file)
{
	if(ramdiskMain)
		panic("%! tried to initialize ramdisk multiple times", "kern");

	ramdiskMain = (g_ramdisk*) heapAllocate(sizeof(g_ramdisk));
	ramdiskParseContents(file);
	logInfo("%! module loaded: %i MB", "ramdisk", file->size / 1024 / 1024);
	logDebug("%! relocated to kernel space: %h -> %h", "ramdisk", module->moduleStart,
			 G_PAGE_ALIGN_UP(module->moduleEnd));
}

void ramdiskParseContents(limine_file* file)
{
	uint8_t* data = (uint8_t*) file->address;
	g_address dataEnd = (g_address) file->address + file->size;

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
			currentHeader = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);
		}
		else
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
		}
		else
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
	g_ramdisk_entry* current = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);
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

	g_ramdisk_entry* currentNode = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);
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

g_ramdisk_entry* ramdiskFindAbsolute(const char* path) { return ramdiskFindRelative(ramdiskMain->root, path); }

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

	g_ramdisk_entry* currentNode = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);
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

	g_ramdisk_entry* currentNode = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);
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

g_ramdisk_entry* ramdiskGetRoot() { return ramdiskMain->root; }

g_ramdisk_entry* ramdiskCreateFile(g_ramdisk_entry* parent, const char* filename)
{
	g_ramdisk_entry* entry = (g_ramdisk_entry*) heapAllocate(sizeof(g_ramdisk_entry));
	entry->next = memoryAtomicLoad(&ramdiskMain->firstEntry, G_MEMORY_ORDER_ACQUIRE);

	int namelen = stringLength(filename);
	entry->name = (char*) heapAllocate(sizeof(char) * (namelen + 1));
	stringCopy(entry->name, filename);

	entry->type = G_RAMDISK_ENTRY_TYPE_FILE;
	entry->id = ramdiskMain->nextUnusedId++;
	entry->parentid = parent->id;

	entry->opens = 0;
	entry->data = nullptr;
	entry->dataSize = 0;
	entry->dataOnRamdisk = false;
	entry->notOnRdBufferLength = 0;

	memoryAtomicStore(&ramdiskMain->firstEntry, entry, G_MEMORY_ORDER_RELEASE);
	return entry;
}

void ramdiskLoadBootModules(limine_module_response* modules)
{
	if(ramdiskMain || !modules || !modules->module_count || modules->module_count > 64)
		panic("%! invalid bootstrap modules", "boot");

	ramdiskMain = new g_ramdisk{};
	ramdiskMain->root = new g_ramdisk_entry{};
	ramdiskMain->root->id = 0;
	ramdiskMain->root->type = G_RAMDISK_ENTRY_TYPE_FOLDER;
	ramdiskMain->nextUnusedId = 1;

	for(uint64_t i = 0; i < modules->module_count; ++i)
	{
		auto file = modules->modules[i];
		if(!file || !file->path || !file->address || file->size > UINT32_MAX || file->path[0] != '/' ||
		   stringLength(file->path) >= G_RAMDISK_MAXIMUM_PATH_LENGTH)
			panic("%! invalid bootstrap file", "boot");

		auto parent = ramdiskMain->root;
		auto path = file->path + 1;
		while(*path)
		{
			char name[G_RAMDISK_MAXIMUM_PATH_LENGTH];
			unsigned length = 0;
			while(path[length] && path[length] != '/')
			{
				name[length] = path[length];
				++length;
			}

			name[length] = 0;

			if(!length || stringEquals(name, ".") || stringEquals(name, ".."))
				panic("%! invalid bootstrap path", "boot");

			auto entry = ramdiskFindChild(parent, name);
			if(path[length])
			{
				if(!entry)
				{
					entry = ramdiskCreateFile(parent, name);
					entry->type = G_RAMDISK_ENTRY_TYPE_FOLDER;
				}

				if(entry->type != G_RAMDISK_ENTRY_TYPE_FOLDER)
					panic("%! conflicting bootstrap path", "boot");

				parent = entry;
				path += length + 1;
			}
			else
			{
				if(entry)
					panic("%! duplicate bootstrap file", "boot");

				entry = ramdiskCreateFile(parent, name);
				entry->data = static_cast<uint8_t*>(file->address);
				entry->dataSize = file->size;
				entry->dataOnRamdisk = true;
				break;
			}
		}
	}

	logDebug("%! loaded %i bootstrap files", "boot", modules->module_count);
}
