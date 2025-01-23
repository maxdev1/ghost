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

#ifndef LIBAHCI_SATA
#define LIBAHCI_SATA

#include <stdint.h>

/**
 * SATA Signatures (ATA/ATAPI Command Set 4)
 */
#define G_SATA_SIGNATURE_ATA		0x00000101	// ATA device
#define G_SATA_SIGNATURE_ATAPI		0xEB140101	// ATAPI device
#define G_SATA_SIGNATURE_HMZ		0xABCD0101	// Host Managed Zone device
#define G_SATA_SIGNATURE_SEMB		0xC33C0101	// SATA Enclosure Management Bridge
#define G_SATA_SIGNATURE_PM			0x96690101	// SATA Port Multiplier

/**
* FIS Type values
*/
#define G_FIS_TYPE_REG_H2D        0x27    // Register Host to Device FIS
#define G_FIS_TYPE_REG_D2H        0x34    // Register Device to Host FIS
#define G_FIS_TYPE_DMA_ACTIVATE   0x39    // DMA Activate FIS - Device to Host
#define G_FIS_TYPE_DMA_SETUP      0x41    // DMA Setup FIS - Bi-directional
#define G_FIS_TYPE_DATA           0x46    // Data FIS - Bi-directional
#define G_FIS_TYPE_BIST_ACTIVATE  0x58    // BIST Activate FIS - Bi-directional
#define G_FIS_TYPE_PIO_SETUP      0x5F    // PIO Setup FIS - Device to Host
#define G_FIS_TYPE_SET_DEVICE     0xA1    // Set Device Bits FIS - Device to Host

/**
 * Register Device to Host FIS
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x34
	uint8_t pmPort: 4;
	uint8_t reserved0: 2;
	uint8_t i: 1; // Interrupt bit
	uint8_t reserved1: 1;
	uint8_t status; // Status register
	uint8_t error; // Error register

	// DW1
	uint8_t lba0; // LBA Low register
	uint8_t lba1; // LBA Mid register
	uint8_t lba2; // LBA High register
	uint8_t device; // Device register

	// DW2
	uint8_t lba3; // LBA expanded
	uint8_t lba4; // LBA expanded
	uint8_t lba5; // LBA expanded
	uint8_t reserved2;

	// DW3
	uint8_t countL; // Sector Count register
	uint8_t countH; // Sector Count register expanded
	uint8_t reserved3[2];

	// DW4
	uint8_t reserved4[4];
} __attribute__((packed)) g_fis_reg_d2h;

/**
 * Set Device Bits - Device to Host FIS
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0xA1
	uint8_t pmPort: 4; // Port Multiplier Port
	uint8_t reserved0: 2;
	uint8_t i: 1; // Interrupt bit
	uint8_t n: 1; // Notification bit
	uint8_t statusL: 3; // Bits 0-2 of the Status register
	uint8_t reserved1: 1;
	uint8_t statusH: 3; // Bits 4-6 of the Status register
	uint8_t reserved2: 1;
	uint8_t error; // Error register

	// DW1
	uint32_t protocolSpecific; // Protocol-specific field
} __attribute__((packed)) g_fis_set_device_bits;


/**
 * DMA Activate - Device to Host
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x39
	uint8_t pmPort: 4; // Port Multiplier Port
	uint8_t reserved0: 4;
	uint8_t reserved1[2];
} __attribute__((packed)) g_fis_dma_activate;


/**
 * DMA Setup - Bi-directional
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x41
	uint8_t pmPort: 4; // Port Multiplier Port
	uint8_t reserved0: 1;
	uint8_t d: 1; // Direction
	uint8_t i: 1; // Interrupt
	uint8_t a: 1; // Auto-Activate
	uint8_t reserved1[2];

	// DW1
	uint32_t dmaBufL; // DMA Buffer Low

	// DW2
	uint32_t dmaBufH; // DMA Buffer High

	// DW3
	uint32_t reserved2;

	// DW4
	uint32_t dmaBufOff; // DMA Buffer Offset

	// DW5
	uint32_t dmaTransferCount; // DMA Transfer Count

	// DW6
	uint32_t reserved3;
} __attribute__((packed)) g_fis_dma_setup;


/**
 * BIST Activate FIS - Bidirectional
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x58
	uint8_t pmPort: 4; // Port Multiplier Port
	uint8_t reserved0: 4;
	uint8_t patternDefinition; // Pattern Definition

	// DW1
	uint32_t data1;

	// DW2
	uint32_t data2;
} __attribute__((packed)) g_fis_bist_activate_fis;


/**
 *  PIO Setup – Device to Host FIS
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x5F
	uint8_t pmPort: 4; // Port Multiplier Port
	uint8_t reserved0: 1;
	uint8_t d: 1; // Direction
	uint8_t i: 1; // Interrupt
	uint8_t reserved1: 1;

	uint8_t status; // Status register
	uint8_t error; // Error register

	// DW1
	uint8_t lba0; // LBA Command Block
	uint8_t lba1; // LBA Command Block
	uint8_t lba2; // LBA Command Block
	uint8_t device; // Device register

	// DW2
	uint8_t lba3; // LBA SRB
	uint8_t lba4; // LBA SRB
	uint8_t lba5; // LBA SRB
	uint8_t reserved2;

	// DW3
	uint8_t countCb; // Count register of Command Block
	uint8_t countSrb; // Count register of Shadow Register Block
	uint8_t reserved3;
	uint8_t eStatus; // New Status register value

	// DW4
	uint16_t transferCount;
	uint16_t reserved4;
} __attribute__((packed)) g_fis_pio_setup_d2h;

/**
 * Register Host to Device FIS
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x27
	uint8_t pmPort: 4;
	uint8_t reserved0: 3;
	uint8_t c: 1; // Command or Control flag
	uint8_t command; // Command register
	uint8_t featuresL; // Features

	// DW1
	uint8_t lba0; // LBA Low register
	uint8_t lba1; // LBA Mid register
	uint8_t lba2; // LBA High register
	uint8_t device; // Device register

	// DW2
	uint8_t lba3; // LBA expanded
	uint8_t lba4; // LBA expanded
	uint8_t lba5; // LBA expanded
	uint8_t featuresH; // Features expanded

	// DW3
	uint8_t countL; // Sector Count register
	uint8_t countH; // Sector Count register expanded
	uint8_t icc; // Isochronous Command Completion
	uint8_t control; // Control register

	// DW4
	uint8_t auxiliary1;
	uint8_t auxiliary2;
	uint8_t reserved1[2];
} __attribute__((packed)) g_fis_reg_h2d;

/**
 * Data FIS - Bi-directional
 */
typedef struct {
	// DW0
	uint8_t type; // Value 0x46
	uint8_t pmport: 4;
	uint8_t reserved0: 4;
	uint8_t reserved1[2];

	// Data follows afterwards
} __attribute__((packed)) g_fis_data;

#endif
