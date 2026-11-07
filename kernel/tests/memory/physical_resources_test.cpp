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

#include "support/fixture.hpp"

namespace
{
constexpr uint64_t G_PAGE_SIZE = 4096;
constexpr uint64_t LIMINE_MEMMAP_USABLE = 0;
constexpr uint64_t LIMINE_MEMMAP_RESERVED = 1;
constexpr uint64_t LIMINE_MEMMAP_ACPI_RECLAIMABLE = 2;
constexpr uint64_t LIMINE_MEMMAP_ACPI_NVS = 3;
constexpr uint64_t LIMINE_MEMMAP_BAD_MEMORY = 4;
constexpr uint64_t LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE = 5;
constexpr uint64_t LIMINE_MEMMAP_KERNEL_AND_MODULES = 6;
constexpr uint64_t LIMINE_MEMMAP_FRAMEBUFFER = 7;
struct limine_memmap_entry { uint64_t base, length, type; };
struct limine_memmap_response { uint64_t entry_count; limine_memmap_entry** entries; };
struct { g_mutex lock; uint32_t freePageCount; } memoryPhysicalAllocator;
uint64_t memoryPhysicalTotal;
#include "production/physicalResources.inc"

TEST(PhysicalResources, CountsRamWithoutReservedFirmwareDevicesOrBadMemory)
{
	const uint64_t types[] = {LIMINE_MEMMAP_USABLE, LIMINE_MEMMAP_RESERVED, LIMINE_MEMMAP_ACPI_RECLAIMABLE,
		LIMINE_MEMMAP_ACPI_NVS, LIMINE_MEMMAP_BAD_MEMORY, LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE,
		LIMINE_MEMMAP_KERNEL_AND_MODULES, LIMINE_MEMMAP_FRAMEBUFFER};
	limine_memmap_entry entries[8];
	limine_memmap_entry* pointers[8];
	for(unsigned i = 0; i < 8; ++i)
	{
		entries[i] = {i * 65536U, 65536, types[i]};
		pointers[i] = &entries[i];
	}
	limine_memmap_response map{8, pointers};
	EXPECT_EQ(5 * 65536U, memoryCountPhysicalBytes(&map));

	memoryPhysicalTotal = memoryCountPhysicalBytes(&map);
	memoryPhysicalAllocator.freePageCount = 7;
	uint64_t total, free;
	memoryGetPhysicalUsage(&total, &free);
	EXPECT_EQ(5 * 65536U, total);
	EXPECT_EQ(7U * G_PAGE_SIZE, free);
	EXPECT_EQ(0, memoryPhysicalAllocator.lock.depth);
}
}
