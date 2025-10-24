/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBAHCI_ATA
#define LIBAHCI_ATA

#include <cstdint>

typedef struct
{
	uint16_t GeneralConfiguration; // 0-1: General Configuration
	uint16_t CylindersLow; // 2-3: Cylinders (Low)
	uint16_t CylindersHigh; // 4-5: Cylinders (High)
	uint16_t Heads; // 6-7: Heads
	uint16_t SectorsPerTrack; // 8-9: Sectors per Track
	uint16_t VendorUnique1[3]; // 10-15: Reserved / Vendor Unique
	uint16_t SerialNumber[10]; // 20-39: Serial Number (20 chars)
	uint16_t BufferType; // 40-41: Buffer Type
	uint16_t BufferSize; // 42-43: Buffer Size (in 512-byte sectors)
	uint16_t ECCSize; // 44-45: ECC Size (in 512-byte sectors)
	uint16_t FirmwareRevision[4]; // 46-49: Firmware Revision
	uint16_t ModelNumber[20]; // 54-93: Model Number (40 chars)
	uint16_t MaximumBlockTransfer; // 94-95: Maximum Block Transfer
	uint16_t MultiwordDMA; // 96-97: Multiword DMA support
	uint16_t PIOMode; // 98-99: PIO Mode
	uint16_t MinMultiwordDMA; // 100-101: Min Multiword DMA
	uint16_t MaxMultiwordDMA; // 102-103: Max Multiword DMA
	uint16_t Reserved2[46]; // 104-255: Reserved
	uint16_t Capabilities; // 256-257: Capabilities
	uint16_t Reserved3; // 258-259: Reserved
	uint16_t CommandSetSupported[2]; // 260-261: Command Set Supported
	uint16_t CommandSetActive[2]; // 262-263: Command Set Active
	uint16_t UltraDMA; // 264-265: Ultra DMA Mode
	uint16_t Reserved4[123]; // 266-511: Reserved
} __attribute__((packed)) identify_device_response_t;

#endif
