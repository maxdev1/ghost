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
#define G_SATA_SIGNATURE_ATA 0x00000101
#define G_SATA_SIGNATURE_ATAPI 0xEB140101
#define G_SATA_SIGNATURE_HMZ 0xABCD0101
#define G_SATA_SIGNATURE_SEMB 0xC33C0101
#define G_SATA_SIGNATURE_PM 0x96690101

/**
 * Frame Information Structure type codes.
 */
#define G_FIS_TYPE_REG_H2D 0x27
#define G_FIS_TYPE_REG_D2H 0x34
#define G_FIS_TYPE_DMA_ACTIVATE 0x39
#define G_FIS_TYPE_DMA_SETUP 0x41
#define G_FIS_TYPE_DATA 0x46
#define G_FIS_TYPE_BIST_ACTIVATE 0x58
#define G_FIS_TYPE_PIO_SETUP 0x5F
#define G_FIS_TYPE_SET_DEVICE 0xA1

/**
 * Register Device to Host FIS
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 2;
	uint8_t i : 1;
	uint8_t reserved1 : 1;
	uint8_t status;
	uint8_t error;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;

	uint8_t countL;
	uint8_t countH;
	uint8_t reserved3[2];

	uint8_t reserved4[4];
} __attribute__((packed)) g_fis_reg_d2h;

/**
 * Set Device Bits - Device to Host FIS
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 2;
	uint8_t i : 1;
	uint8_t n : 1;
	uint8_t statusL : 3;
	uint8_t reserved1 : 1;
	uint8_t statusH : 3;
	uint8_t reserved2 : 1;
	uint8_t error;

	uint32_t protocolSpecific;
} __attribute__((packed)) g_fis_set_device_bits;

/**
 * DMA Activate - Device to Host
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 4;
	uint8_t reserved1[2];
} __attribute__((packed)) g_fis_dma_activate;

/**
 * DMA Setup - Bi-directional
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 1;
	uint8_t d : 1;
	uint8_t i : 1;
	uint8_t a : 1;
	uint8_t reserved1[2];

	uint32_t dmaBufL;

	uint32_t dmaBufH;

	uint32_t reserved2;

	uint32_t dmaBufOff;

	uint32_t dmaTransferCount;

	uint32_t reserved3;
} __attribute__((packed)) g_fis_dma_setup;

/**
 * BIST Activate FIS - Bidirectional
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 4;
	uint8_t patternDefinition;

	uint32_t data1;

	uint32_t data2;
} __attribute__((packed)) g_fis_bist_activate_fis;

/**
 *  PIO Setup – Device to Host FIS
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 1;
	uint8_t d : 1;
	uint8_t i : 1;
	uint8_t reserved1 : 1;

	uint8_t status;
	uint8_t error;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;

	uint8_t countCb;
	uint8_t countSrb;
	uint8_t reserved3;
	uint8_t eStatus;

	uint16_t transferCount;
	uint16_t reserved4;
} __attribute__((packed)) g_fis_pio_setup_d2h;

/**
 * Register Host to Device FIS. c selects a command rather than control update;
 * lba0 through lba5 encode the six-byte LBA and countL/countH encode sectors.
 */
typedef struct
{

	uint8_t type;
	uint8_t pmPort : 4;
	uint8_t reserved0 : 3;
	uint8_t c : 1;
	uint8_t command;
	uint8_t featuresL;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t featuresH;

	uint8_t countL;
	uint8_t countH;
	uint8_t icc;
	uint8_t control;

	uint8_t auxiliary1;
	uint8_t auxiliary2;
	uint8_t reserved1[2];
} __attribute__((packed)) g_fis_reg_h2d;

/**
 * Data FIS - Bi-directional
 */
typedef struct
{

	uint8_t type;
	uint8_t pmport : 4;
	uint8_t reserved0 : 4;
	uint8_t reserved1[2];

} __attribute__((packed)) g_fis_data;

#endif
