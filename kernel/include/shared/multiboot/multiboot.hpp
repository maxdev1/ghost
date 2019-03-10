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

#ifndef __MULTIBOOT__
#define __MULTIBOOT__

#include "ghost/stdint.h"

#define G_MULTIBOOT_HEADER_MAGIC		0x1BADB002
#define G_MULTIBOOT_BOOTLOADER_MAGIC	0x2BADB002

/**
 * a.out symbol table
 */
struct g_multiboot_aout_symbol_table
{
	uint32_t tabSize;
	uint32_t strSize;
	uint32_t address;
	uint32_t reserved;
}__attribute__((packed));

/**
 * ELF section header
 */
struct g_multiboot_elf_section_header_table
{
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
}__attribute__((packed));

/**
 * Structure of the multiboot information. This struct is
 * built by GRUB somewhere in memory, and a pointer to it is
 * passed to the multiboot-compliant loader assembler stub.
 */
struct g_multiboot_information
{
	uint32_t flags;
	uint32_t memoryLower;
	uint32_t memoryUpper;
	uint32_t bootDevice;
	uint32_t cmdline;
	uint32_t modulesCount;
	uint32_t modulesAddress;
	union
	{
		g_multiboot_aout_symbol_table aoutSymbolTable;
		g_multiboot_elf_section_header_table elfSectionHeaderTable;
	} tables;
	uint32_t memoryMapLength;
	uint32_t memoryMapAddress;
}__attribute__((packed));

/**
 * Structure of a multiboot module.
 */
struct g_multiboot_module
{
	uint32_t moduleStart;
	uint32_t moduleEnd;
	char *path;
	uint32_t reserved;
}__attribute__((packed));

/**
 * Structure of one entry in the memory map.
 */
struct g_multiboot_mmap
{
	uint32_t size;
	uint32_t baseAddressLower;
	uint32_t baseAddressHigher;
	uint32_t lengthLower;
	uint32_t lengthHigher;
	uint32_t type;
}__attribute__((packed));

/**
 * Looks for the module with the given path.
 *
 * @param info multiboot information structure
 * @param path module path
 * @return the module or null
 */
g_multiboot_module* multibootFindModule(g_multiboot_information* info, const char* path);

#endif
