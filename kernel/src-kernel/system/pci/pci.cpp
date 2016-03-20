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

#include "pci.hpp"
#include "logger/logger.hpp"
#include "system/io_ports.hpp"
#include "utils/list_entry.hpp"

static g_list_entry<g_pci_header*>* pci_headers = 0;
static uint32_t pci_headers_found = 0;

/**
 *
 */
void g_pci::initialize() {

	scanBusesByBruteforce();
	g_log_info("%! found %i devices", "pci", pci_headers_found);
}

/**
 *
 */
g_pci_header* g_pci::findDeviceAt(uint8_t bus, uint8_t slot, uint8_t function) {

	g_list_entry<g_pci_header*>* next = pci_headers;
	while (next) {
		g_pci_header* header = next->value;
		if (header->bus == bus && header->slot == slot && header->function == function) {
			return header;
		}
		next = next->next;
	}

	return 0;
}

/**
 * Searches for a device of the requested type.
 */
g_pci_header* g_pci::findDeviceOfType(uint8_t classCode, uint8_t subclassCode, int index) {

	int pos = 0;
	g_list_entry<g_pci_header*>* next = pci_headers;
	while (next) {

		g_pci_header* header = next->value;
		if (classCode == header->classCode && subclassCode == header->subclassCode) {
			if (pos == index) {
				return header;
			}
			pos++;
		}
		next = next->next;
	}

	return 0;
}

/**
 *
 */
void g_pci::scanBusesByBruteforce() {
	g_log_info("%! scanning bus with brute force", "pci");

	for (uint32_t bus = 0; bus < 256; bus++) {
		for (uint32_t slot = 0; slot < 32; slot++) {

			// check for a valid device in function 0
			if (isValidDevice(bus, slot, 0)) {

				// check if it's a multifunction device
				if (isMultifunctionDevice(bus, slot)) {

					// device may have up to 8 functions, check each
					for (int function = 0; function < 8; function++) {

						// check if function is valid
						if (isValidDevice(bus, slot, function)) {
							createDeviceHeader(bus, slot, function);
						}
					}

				} else {
					createDeviceHeader(bus, slot, 0);
				}
			}
		}
	}
}

/**
 *
 */
void g_pci::createDeviceHeader(uint8_t bus, uint8_t slot, uint8_t function) {

	// read header type
	g_pci_header* header = createHeader();
	header->bus = bus;
	header->slot = slot;
	header->function = function;

	uint8_t headerTypeWithMultifuncBit = (readConfiguration(bus, slot, function, 0x0C) & 0xFF0000) >> 16;
	header->headerType = headerTypeWithMultifuncBit & 0x7F;
	header->multiFunction = (headerTypeWithMultifuncBit & 0x80) >> 7;

	// read information common to all headers
	uint32_t entry00 = readConfiguration(bus, slot, function, 0x00);
	header->deviceId = entry00 >> 16;
	header->vendorId = entry00 & 0xFFFF;

	uint32_t entry08 = readConfiguration(bus, slot, function, 0x08);
	header->classCode = (entry08 & 0xFF000000) >> 24;
	header->subclassCode = (entry08 & 0xFF0000) >> 16;
	header->progIf = (entry08 & 0xFF00) >> 8;
	header->revisionId = (entry08 & 0xFF);
}

/**
 *
 */
uint32_t g_pci::readConfiguration(uint8_t bus, uint8_t slot, uint8_t function, uint8_t reg) {

	uint32_t address = (((uint32_t) bus << 16) | ((uint32_t) slot << 11) | ((uint32_t) function << 8) | (reg & 0xFC) | ENABLE_BIT);

	// write address to address field
	g_io_ports::writeInt(CONFIG_ADDRESS, address);

	// read data from data field
	return g_io_ports::readInt(CONFIG_DATA);
}

/**
 *
 */
bool g_pci::isValidDevice(uint8_t bus, uint8_t slot, uint8_t function) {

	uint16_t vendor = readConfiguration(bus, slot, function, 0) & 0xFFFF;

	// if invalid vendor, this is not a valid device
	if (vendor == 0xFFFF) {
		return false;
	}

	return true;
}

/**
 *
 */
bool g_pci::isMultifunctionDevice(uint8_t bus, uint8_t slot) {

	uint8_t headerType = (readConfiguration(bus, slot, 0, 0x0C) & 0xFF0000) >> 16;
	return (headerType & 0x80);
}

/**
 *
 */
g_pci_header* g_pci::createHeader() {

	g_pci_header* header = new g_pci_header();

	g_list_entry<g_pci_header*>* entry = new g_list_entry<g_pci_header*>();
	entry->next = pci_headers;
	entry->value = header;
	pci_headers = entry;

	pci_headers_found++;

	return header;
}

/**
 *
 */
uint32_t g_pci::getDeviceCount() {
	return pci_headers_found;
}

/**
 *
 */
g_pci_header* g_pci::getDeviceAt(uint32_t position) {

	uint32_t pos = 0;
	g_list_entry<g_pci_header*>* next = pci_headers;
	while (next) {
		if (position == pos) {
			return next->value;
		}
		++pos;
		next = next->next;
	}

	return nullptr;
}

