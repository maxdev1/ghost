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

#include <system/interrupts/ioapic.hpp>

#include <memory/address_space.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>

/**
 *
 */
void g_ioapic::createMapping() {
	// Get a virtual range for mapping
	virtualAddress = g_kernel::virtual_range_pool->allocate(2);
	if (virtualAddress == 0) {
		g_kernel::panic("%! could not get a virtual range for mapping", "ioapic");
	}

	// Add the physical offset to the virtual address
	virtualAddress += physicalAddress - G_PAGE_ALIGN_DOWN(physicalAddress);

	// Map the two pages
	g_log_debug("%! mapped at %h (phys %h)", "ioapic", virtualAddress, physicalAddress);
	g_address_space::map(G_PAGE_ALIGN_DOWN(virtualAddress), G_PAGE_ALIGN_DOWN(physicalAddress), DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	g_address_space::map(G_PAGE_ALIGN_DOWN(virtualAddress) + G_PAGE_SIZE, G_PAGE_ALIGN_DOWN(physicalAddress) + G_PAGE_SIZE, DEFAULT_KERNEL_TABLE_FLAGS,
	DEFAULT_KERNEL_PAGE_FLAGS);
}

/**
 *
 */
uint32_t g_ioapic::read(uint32_t reg) {
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = reg;
	return *((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN));
}

/**
 *
 */
void g_ioapic::write(uint32_t reg, uint32_t value) {
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = reg;
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN)) = value;
}

/**
 *
 */
uint64_t g_ioapic::getRedirectionEntry(uint32_t index) {
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2;
	uint64_t lo = *((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN));
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2 + 1;
	uint64_t hi = *((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN));

	return (hi << 32) | lo;
}

/**
 *
 */
void g_ioapic::setRedirectionEntry(uint32_t index, uint64_t value) {
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2;
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN)) = value & 0xFFFFFFFF;
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2 + 1;
	*((volatile uint32_t*) (virtualAddress + IOAPIC_REGWIN)) = value >> 32;
}

/**
 *
 */
void g_ioapic::initialize() {
	createMapping();

	// Get ID
	uint32_t idValue = read(IOAPIC_ID);
	uint32_t reportedId = (idValue >> 24) & 0xF;

	// If not right ID, reprogram it
	if (reportedId != id) {
		g_log_warn("%! has different ID (%i) than what ACPI reported (%i), reprogramming", "ioapic", idValue, reportedId);

		// Remove the actual ID bits
		idValue &= ~(0xF << 24);
		// Set new ID bits
		idValue |= (id & 0xF) << 24;
		// Write value
		write(IOAPIC_ID, idValue);
	}

	// Get version
	uint32_t versionValue = read(IOAPIC_VER);
	uint32_t version = versionValue & 0xFF;
	redirectEntryCount = (versionValue >> 16) & 0xFF;
	g_log_info("%! id %i: version %i, redirect entries: %i", "ioapic", id, version, redirectEntryCount);
}

/**
 *
 */
void g_ioapic::mask(uint32_t source) {
	uint32_t entryIndex = source - globalSystemInterruptBase;
	uint64_t entry = getRedirectionEntry(entryIndex);

	entry &= ~(IOAPIC_REDTBL_MASK_INTMASK);
	entry |= IOAPIC_REDTBL_INTMASK_MASKED;

	setRedirectionEntry(entryIndex, entry);
}

/**
 *
 */
void g_ioapic::unmask(uint32_t source) {
	uint32_t entryIndex = source - globalSystemInterruptBase;
	uint64_t entry = getRedirectionEntry(entryIndex);

	entry &= ~(IOAPIC_REDTBL_MASK_INTMASK);
	entry |= IOAPIC_REDTBL_INTMASK_UNMASKED;

	setRedirectionEntry(entryIndex, entry);
}
