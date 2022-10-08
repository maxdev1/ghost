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

#ifndef GHOST_API_CALLS_MEMORYCALLS
#define GHOST_API_CALLS_MEMORYCALLS

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "ghost/types.h"

/**
 * @field size
 * 		the required size in bytes
 *
 * @field virtualResult
 * 		the virtual address of the allocated area in the current processes
 * 		address space. this address is page-aligned. if allocation
 * 		fails, this field is 0.
 *
 * @security-level APPLICATION
 */
typedef struct {
	uint32_t size;

	void* virtualResult;
}__attribute__((packed)) g_syscall_alloc_mem;

/**
 * @field memory
 * 		the memory area to share
 *
 * @field size
 *		the minimum size to share
 *
 * @field processId
 *		the id of the target process
 *
 * @field virtualAddress
 * 		the resulting page-aligned virtual address in the target
 * 		processes address space. if sharing fails, this field is 0.
 *
 * @security-level APPLICATION
 */
typedef struct {
	void* memory;
	uint32_t size;
	g_pid processId;

	void* virtualAddress;
}__attribute__((packed)) g_syscall_share_mem;

/**
 * @field physicalAddress
 * 		the page-aligned physical address of the area
 *
 * @field size
 * 		the minimum size to map
 *
 * @field virtualAddress
 * 		the resulting page-aligned virtual address in the current
 * 		processes address space. if mapping fails, this field is 0.
 *
 * @security-level DRIVER
 */
typedef struct {
	g_physical_address physicalAddress;
	uint32_t size;

	void* virtualAddress;
}__attribute__((packed)) g_syscall_map_mmio;

/**
 * @field virtualBase
 * 		the address of the area to free
 *
 * @security-level APPLICATION
 */
typedef struct {
	uint32_t virtualBase;
}__attribute__((packed)) g_syscall_unmap;

/**
 * @field size
 * 		the size to allocate
 *
 * @field result
 * 		the resulting area address
 *
 * @security-level DRIVER
 */
typedef struct {
	uint32_t size;

	void* result;
}__attribute__((packed)) g_syscall_lower_malloc;

/**
 * @field memory
 * 		the memory to free
 *
 * @security-level DRIVER
 */
typedef struct {
	void* memory;
}__attribute__((packed)) g_syscall_lower_free;

/**
 * @field amount
 * 		the amount to increase/decrease the heap
 *
 * @field address
 * 		the previous heap break result (as expected by sbrk)
 *
 * @field successful
 * 		whether or not the call was successful
 *
 * @security-level APPLICATION
 */
typedef struct {
	int32_t amount;

	uint32_t address;
	uint8_t successful;
}__attribute__((packed)) g_syscall_sbrk;

#endif
