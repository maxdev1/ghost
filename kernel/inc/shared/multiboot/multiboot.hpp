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

#define G_MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define G_MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/**
 * a.out symbol table
 */
struct g_multiboot_aout_symbol_table
{
	uint32_t tabSize;
	uint32_t strSize;
	uint32_t address;
	uint32_t reserved;
} __attribute__((packed));

/**
 * ELF section header
 */
struct g_multiboot_elf_section_header_table
{
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
} __attribute__((packed));

/**
 * Structure of a multiboot module.
 */
struct g_multiboot_module
{
	uint32_t moduleStart;
	uint32_t moduleEnd;
	char* path;
	uint32_t reserved;
} __attribute__((packed));

/**
 * Structure of one entry in the memory map.
 */
struct g_multiboot_mmap
{
	uint32_t size;
	uint64_t baseAddress;
	uint64_t length;
	uint32_t type;
} __attribute__((packed));

#define G_MULTIBOOT_MMAP_TYPE_FREE		1
#define G_MULTIBOOT_MMAP_TYPE_ACPI		3
#define G_MULTIBOOT_MMAP_TYPE_RESERVED	4
#define G_MULTIBOOT_MMAP_TYPE_DEFECT	5


struct g_multiboot_drives
{
	uint32_t size;
	uint8_t number;
	uint8_t mode;
	uint16_t cylinders;
	uint8_t heads;
	uint8_t sectors;
	uint16_t ports; // Array
} __attribute__((packed));

#define G_MULTIBOOT_DRIVES_MODE_CHS 0
#define G_MULTIBOOT_DRIVES_MODE_LBA 1

struct g_multiboot_apm
{
	uint16_t version;
	uint16_t cseg;
	uint32_t offset;
	uint16_t cseg16;
	uint16_t dseg;
	uint16_t flags;
	uint16_t csegLen;
	uint16_t cseg16Len;
	uint16_t dsegLen;
} __attribute__((packed));

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
	g_multiboot_module* modules;

	union
	{
		g_multiboot_aout_symbol_table aoutSymbolTable;
		g_multiboot_elf_section_header_table elfSectionHeaderTable;
	} tables;

	uint32_t memoryMapLength;
	g_multiboot_mmap* memoryMap;

	uint32_t drivesLength;
	g_multiboot_drives* drives;

	void* configTable;
	const char* bootloaderName;
	g_multiboot_apm* apm;

	// Followed by VBE and Framebuffer structures
} __attribute__((packed));

#define G_MULTIBOOT_FLAGS_LOWERMEM (1 << 0)
#define G_MULTIBOOT_FLAGS_BOOTDEV (1 << 1)
#define G_MULTIBOOT_FLAGS_CMDLINE (1 << 2)
#define G_MULTIBOOT_FLAGS_MODS (1 << 3)
#define G_MULTIBOOT_FLAGS_IS_AOUT (1 << 4)
#define G_MULTIBOOT_FLAGS_IS_ELF (1 << 5)
#define G_MULTIBOOT_FLAGS_MMAP (1 << 6)
#define G_MULTIBOOT_FLAGS_DRIVES (1 << 7)
#define G_MULTIBOOT_FLAGS_CONFIGS (1 << 8)
#define G_MULTIBOOT_FLAGS_BOOTLDNAM (1 << 9)
#define G_MULTIBOOT_FLAGS_APM (1 << 10)
#define G_MULTIBOOT_FLAGS_VBE (1 << 11)
#define G_MULTIBOOT_FLAGS_FRAMEBUF (1 << 12)

/**
 * Looks for the module with the given path.
 *
 * @param info multiboot information structure
 * @param path module path
 * @return the module or null
 */
g_multiboot_module* multibootFindModule(g_multiboot_information* info, const char* path);

#endif
