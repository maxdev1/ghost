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

#ifndef LIBAHCI_AHCI
#define LIBAHCI_AHCI

#include "sata.hpp"

#include <stdint.h>
#include <stddef.h>

/**
 * Per-port Host Bus Adapter registers. Offsets are relative to the port base;
 * consecutive ports are separated by 0x80 bytes.
 */
typedef struct
{
	uint32_t clb;
	uint32_t clbu;
	uint32_t fb;
	uint32_t fbu;
	uint32_t is;
	uint32_t ie;
	uint32_t cmd;
	uint32_t reserved0;
	uint32_t tfd;
	uint32_t sig;
	uint32_t ssts;
	uint32_t sctl;
	uint32_t serr;
	uint32_t sact;
	uint32_t ci;
	uint32_t sntf;
	uint32_t fbs;
	uint32_t reserved1[11];
	uint32_t vendor[4];
} g_ahci_hba_port;

/**
 * HBAxPORT.SSTS.DET
 */
#define G_AHCI_HBA_PORT_SSTS_DET_NONE 0
#define G_AHCI_HBA_PORT_SSTS_DET_PRESENT 1
#define G_AHCI_HBA_PORT_SSTS_DET_READY 3
#define G_AHCI_HBA_PORT_SSTS_DET_OFFLINE 4

typedef struct
{
	uint32_t cap;
	uint32_t ghc;
	uint32_t is;
	uint32_t pi;
	uint32_t vs;
	uint32_t cccCtl;
	uint32_t cccPorts;
	uint32_t emLoc;
	uint32_t emCtl;
	uint32_t cap2;
	uint32_t bohc;
} g_ahci_hba_ghc;

#define G_AHCI_HBA_PORT_OFFSET 0x100

/**
 * One command-list entry. cfl counts command FIS DWORDs; w selects a write.
 * ctba/ctbau locate the command table, and prdtl counts its PRDT entries.
 */
typedef struct
{

	uint8_t cfl : 5;
	uint8_t a : 1;
	uint8_t w : 1;
	uint8_t p : 1;

	uint8_t r : 1;
	uint8_t b : 1;
	uint8_t c : 1;
	uint8_t reserved0 : 1;
	uint8_t pmp : 4;

	uint16_t prdtl;

	uint32_t prdbc;

	uint32_t ctba;

	uint32_t ctbau;

	uint32_t reserved1[4];
} __attribute__((packed)) g_hba_command_header;

/**
 * Command Table
 */
typedef struct
{
	uint8_t cfis[64];
	uint8_t acmd[16];
} __attribute__((packed)) g_hba_command_table;

#define G_HBA_COMMAND_TABLE_PRDT_OFFSET 0x80

/**
 * Physical Region Descriptor Table entry. dba/dbau locate the DMA region,
 * dbc stores its byte count minus one, and i requests completion interrupts.
 */
typedef struct
{

	uint32_t dba;

	uint32_t dbau;

	uint32_t reserved0;

	uint32_t dbc : 22;
	uint32_t reserved : 9;
	uint32_t i : 1;
} __attribute__((packed)) g_hba_prdt_entry;

#define G_HBA_PRDT_MAX_ENTRIES 256

/**
 * Received FIS area. Padding preserves the AHCI-defined offsets for each FIS.
 */
typedef struct
{
	g_fis_dma_setup dsfis;
	uint8_t pad0[4];

	g_fis_pio_setup_d2h psfis;
	uint8_t pad1[12];

	g_fis_reg_d2h rfis;
	uint8_t pad2[4];

	g_fis_set_device_bits sdbfis;

	uint8_t ufis[64];

	uint8_t reserved0[0x100 - 0xA0];
} __attribute__((packed)) g_hba_fis;

#endif
