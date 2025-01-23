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

// As per Serial ATA AHCI specification v1.3.1
// https://www.intel.com/content/www/us/en/io/serial-ata/serial-ata-ahci-spec-rev1-3-1.html

/**
 * Generic Host Control structure of the Host Bus Adapter (HBA)
*/
typedef struct {
	uint32_t clb; // Command List Base Address
	uint32_t clbu; // Command List Base Address Upper 32-Bit
	uint32_t fb; // FIS Base Address
	uint32_t fbu; // FIS Base Address Upper 32-Bits
	uint32_t is; // Interrupt Status
	uint32_t ie; // Interrupt Enable
	struct {
		uint8_t st: 1; // Start
		uint8_t sud: 1; // Spin-Up Device
		uint8_t pod: 1; // Power On Device
		uint8_t clo: 1; // Command List Override
		uint8_t fre: 1; // FIS Receive Enable
		uint8_t reserved0: 3;
		uint8_t ccs: 5; // Current Command Slot
		uint8_t mpss: 1; // Mechanical Presence Switch State
		uint8_t fr: 1; // FIS REceive Running
		uint8_t cr: 1; // Command List Running
		uint8_t cps: 1; // Cold Presence State
		uint8_t pma: 1; // Port Multiplier Attached
		uint8_t hpcp: 1; // Hot Plug Capable Port
		uint8_t mpsp: 1; // Mechanical Presence Switch Attached to Port
		uint8_t cpd: 1; // Cold Presence Detection
		uint8_t esp: 1; // External SATA Port
		uint8_t fbscp: 1; // FIS-based Switching Capable Port
		uint8_t apste: 1; // Automatic Partial to Slumber Transitions Enabled
		uint8_t atapi: 1; // Device is ATAPI
		uint8_t dlae: 1; // Drive LED on ATAPI Enable
		uint8_t alpe: 1; // Aggressive Link Power Management Enable
		uint8_t asp: 1; // Aggressive Slumber / Partial
		uint8_t icc: 4; // Interface Communication Control
	} __attribute__((packed)) cmd; // Command and Status
	uint32_t reserved0;
	uint32_t tfd; // Task File Data
	uint32_t sig; // Signature
	struct {
		uint8_t det: 4; // Device Detection
		uint8_t spd: 4; // Current Interface Speed
		uint8_t ipm: 4; // Interface Power Management
		uint32_t reserved0: 20;
	} __attribute__((packed)) ssts; // Serial ATA Status
	uint32_t sctl; // Serial ATA Control
	uint32_t serr; // Serial ATA Error
	uint32_t sact; // Serial ATA Active
	uint32_t ci; // Command Issue
	uint32_t sntf; // Serial ATA Notification
	uint32_t fbs; // FIS-based Switching Control
	uint32_t reserved1[11];
	uint32_t vendor[4]; // Vendor Specific
} __attribute__((packed)) g_ahci_hba_port;

/**
 * HBAxPORT.SSTS.DET
 */
#define G_AHCI_HBA_PORT_SSTS_DET_NONE		0 // No device detected and Phy communication not established
#define G_AHCI_HBA_PORT_SSTS_DET_PRESENT	1 // Device presence detected but Phy communication not established
#define G_AHCI_HBA_PORT_SSTS_DET_READY		3 // Device presence detected and Phy communication established
#define G_AHCI_HBA_PORT_SSTS_DET_OFFLINE	4 // Phy in offline mode as a result of the interface being disabled or running in a BIST loopback mode


typedef struct {
	uint32_t cap; // Host Capabilities
	struct {
		uint32_t hbaReset: 1; // Bit 0
		uint32_t interruptEnable: 1; // Bit 1
		uint32_t reserved: 29; // Bits 2-30
		uint32_t ahciEnable: 1; // Bit 31
	} __attribute__((packed)) ghc; // Global Host Control
	uint32_t is; // Interrupt Status
	uint32_t pi; // Ports implemented
	uint32_t vs; // Version
	uint32_t cccCtl; // Command Completion Coalescing Control
	uint32_t cccPorts; // Command Completion Coalsecing Ports
	uint32_t emLoc; // Enclosure Management Location
	uint32_t emCtl; // Enclosure Management Control
	uint32_t cap2; // Host Capabilities Extended
	uint32_t bohc; // BIOS/OS Handoff Control and Status
} __attribute__((packed)) g_ahci_hba_ghc;

#define G_AHCI_HBA_PORT_OFFSET 0x100

/**
 * Command Header
 */
typedef struct {
	// DW0
	uint8_t cfl: 5; // Command FIS Length
	uint8_t a: 1; // ATAPI
	uint8_t w: 1; // Write, 1: H2D, 0: D2H
	uint8_t p: 1; // Prefetchable

	uint8_t r: 1; // Reset
	uint8_t b: 1; // BIST
	uint8_t c: 1; // Clear Busy upon R_OK
	uint8_t reserved0: 1;
	uint8_t pmp: 4; // Port Multiplier Port

	uint16_t prdtl; // Physical Region Descriptor Table Length

	// DW1
	uint32_t prdbc; // Physical Region Descriptor Byte Count

	// DW2
	uint32_t ctba; // Command Table Descriptor Base Address

	// DW3
	uint32_t ctbau; // Command Table Descriptor Base Address Upper 32-bits

	// DW4-7
	uint32_t reserved1[4];
} __attribute__((packed)) g_hba_command_header;

/**
 * Command Table
 */
typedef struct {
	uint8_t cfis[64];
	uint8_t acmd[16];
} __attribute__((packed)) g_hba_command_table;

#define G_HBA_COMMAND_TABLE_PRDT_OFFSET		0x80

/**
 * Physical Region Descriptor Table (PRDT)
 */
typedef struct {
	// DW0
	uint32_t dba; // Data base address (lower 32 bits)

	// DW1
	uint32_t dbau; // Data base address (upper 32 bits)

	// DW2
	uint32_t reserved0;

	// DW3
	uint32_t dbc: 22; // Data Byte Count
	uint32_t reserved: 9;
	uint32_t i: 1; // Interrupt on Completion
} __attribute__((packed)) g_hba_prdt_entry;

#define G_HBA_PRDT_MAX_ENTRIES	 256


/**
 * Received FIS structure
 */
typedef struct {
	g_fis_dma_setup dsfis; // DMA Setup FIS
	uint8_t pad0[4];

	g_fis_pio_setup_d2h psfis; // PIO Setup FIS
	uint8_t pad1[12];

	g_fis_reg_d2h rfis; // D2H Register FIS
	uint8_t pad2[4];

	g_fis_set_device_bits sdbfis; // Set Device Bits FIS

	uint8_t ufis[64]; // Unknown FIS

	uint8_t reserved0[0x100 - 0xA0];
} __attribute__((packed)) g_hba_fis;

#endif
