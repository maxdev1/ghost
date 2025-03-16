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

#ifndef __PCI__
#define __PCI__

#define PCI_NUM_BUSES		256
#define PCI_NUM_DEVICES		32
#define PCI_NUM_FUNCTIONS	8


#define PCI_CONFIG_OFF(bus, device, function, offset)	((1 << 31) | (bus << 16) | (device << 11) | (function << 8) | offset)

/**
* If bit 1 of a BAR is set, it is IO related.
*/
#define PCI_CONFIG_BAR_IO	0x1

/**
* IO ports
*/
#define PCI_CONFIG_PORT_ADDR 0xCF8
#define PCI_CONFIG_PORT_DATA 0xCFC


/**
* Configuration space offsets
*/
#define PCI_CONFIG_OFF_VENDOR_ID			0x00
#define PCI_CONFIG_OFF_DEVICE_ID			0x02
#define PCI_CONFIG_OFF_COMMAND				0x04
#define PCI_CONFIG_OFF_STATUS				0x08
#define PCI_CONFIG_OFF_REVISION				0x08
#define PCI_CONFIG_OFF_CLASS				0x09
#define PCI_CONFIG_OFF_HEADERS				0x0C
#define PCI_CONFIG_OFF_BAR0					0x10
#define PCI_CONFIG_OFF_BAR1					0x14
#define PCI_CONFIG_OFF_BAR2					0x18
#define PCI_CONFIG_OFF_BAR3					0x1C
#define PCI_CONFIG_OFF_BAR4					0x20
#define PCI_CONFIG_OFF_BAR5					0x24
#define PCI_CONFIG_OFF_CARDBUS				0x28
#define PCI_CONFIG_OFF_SS					0x2C
#define PCI_CONFIG_OFF_EROM 				0x30
#define PCI_CONFIG_OFF_CAP					0x34
#define PCI_CONFIG_OFF_INTR					0x3C
#define PCI_CONFIG_OFF_MGNT					0x3E
#define PCI_CONFIG_OFF_MLAT					0x3F

/**
* Class definitions (https://pcisig.com/sites/default/files/files/PCI_Code-ID_r_1_11__v24_Jan_2019.pdf)
*/
#define PCI_BASE_CLASS_UNKNOWN              0x00
#define PCI_BASE_CLASS_MASS_STORAGE         0x01
#define PCI_BASE_CLASS_NETWORK              0x02
#define PCI_BASE_CLASS_DISPLAY              0x03
#define PCI_BASE_CLASS_MULTIMEDIA           0x04
#define PCI_BASE_CLASS_MEMORY_CONTROLLER    0x05
#define PCI_BASE_CLASS_BRIDGE               0x06
#define PCI_BASE_CLASS_SIMPLE_COMMUNICATION 0x07
#define PCI_BASE_CLASS_BASE_SYSTEM          0x08
#define PCI_BASE_CLASS_INPUT_DEVICE         0x09
#define PCI_BASE_CLASS_DOCKING_STATION      0x0A
#define PCI_BASE_CLASS_PROCESSOR            0x0B
#define PCI_BASE_CLASS_SERIAL_BUS           0x0C
#define PCI_BASE_CLASS_WIRELESS             0x0D
#define PCI_BASE_CLASS_INTELLIGENT_IO       0x0E
#define PCI_BASE_CLASS_SATELLITE_COMM       0x0F
#define PCI_BASE_CLASS_ENCRYPTION           0x10
#define PCI_BASE_CLASS_DATA_ACQUISITION     0x11
#define PCI_BASE_CLASS_PROCESSING_ACCEL     0x12
#define PCI_BASE_CLASS_NON_ESSENTIAL_INST   0x13
// 0x14 - 0xFE are reserved
#define PCI_BASE_CLASS_UNDEFINED            0xFF

#define PCI_01_SUBCLASS_SCSI                0x00
#define PCI_01_SUBCLASS_IDE					0x01
#define PCI_01_SUBCLASS_FLOPPY				0x02
#define PCI_01_SUBCLASS_IPI					0x03
#define PCI_01_SUBCLASS_RAID				0x04
#define PCI_01_SUBCLASS_ATA					0x05
#define PCI_01_SUBCLASS_SATA				0x06
#define PCI_01_SUBCLASS_SAS					0x07
#define PCI_01_SUBCLASS_NVM					0x08
#define PCI_01_SUBCLASS_UFS					0x09
#define PCI_01_SUBCLASS_OTHER				0x80

#define PCI_01_06_PROGIF_VENDOR_SPEC		0x00
#define PCI_01_06_PROGIF_AHCI				0x01
#define PCI_01_06_PROGIF_STORAGEBUS			0x02

#define PCI_03_SUBCLASS_VGA                 0x00
#define PCI_03_SUBCLASS_XGA                 0x01
#define PCI_03_SUBCLASS_3D                  0x02
#define PCI_03_SUBCLASS_OTHER               0x80

#define PCI_03_00_PROGIF_VGA_COMPATIBLE     0x00
#define PCI_03_00_PROGIF_8514_COMPATIBLE    0x01

#endif
