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

#include "list.hpp"

#include <stdio.h>
#include <ghost/kernquery.h>


/**
 * Table of definitions
 */
PCIClassMapping PCI_CLASSCODE_MAPPINGS[] = {
	{ 0x00, 0x00, 0x00, "Other pre-2.0 device", "Other" },
	{ 0x00, 0x01, 0x00, "Other pre-2.0 device", "VGA Compatible" },

	{ 0x01, 0x00, 0x00, "Mass Storage Controller", "SCSI Bus Controller" },
	{ 0x01, 0x01, 0x00, "Mass Storage Controller", "IDE Controller" },
	{ 0x01, 0x02, 0x00, "Mass Storage Controller", "Floppy Disk Controller" },
	{ 0x01, 0x03, 0x00, "Mass Storage Controller", "IPI Bus Controller" },
	{ 0x01, 0x04, 0x00, "Mass Storage Controller", "RAID Controller" },
	{ 0x01, 0x05, 0x20, "Mass Storage Controller", "ATA Controller (Single DMA)" },
	{ 0x01, 0x05, 0x30, "Mass Storage Controller", "ATA Controller (Chained DMA)" },
	{ 0x01, 0x06, 0x00, "Mass Storage Controller", "SATA (Vendor Spec. Interface)" },
	{ 0x01, 0x06, 0x01, "Mass Storage Controller", "SATA (AHCI 1.0)" },
	{ 0x01, 0x07, 0x00, "Mass Storage Controller", "Serial Attached SCSI (SAS)" },
	{ 0x01, 0x80, 0x00, "Mass Storage Controller", "Other" },

	{ 0x02, 0x00, 0x00, "Network Controller", "Ethernet Controller" },
	{ 0x02, 0x01, 0x00, "Network Controller", "Token Ring Controller" },
	{ 0x02, 0x02, 0x00, "Network Controller", "FDDI Controller" },
	{ 0x02, 0x03, 0x00, "Network Controller", "ATM Controller" },
	{ 0x02, 0x04, 0x00, "Network Controller", "ISDN Controller" },
	{ 0x02, 0x05, 0x00, "Network Controller", "WorldFip Controller" },
	{ 0x02, 0x06, 0x00, "Network Controller", "PICMG 2.14 Multi Computing" },
	{ 0x02, 0x80, 0x00, "Network Controller", "Other" },

	{ 0x03, 0x00, 0x00, "Display Controller", "VGA-Compatible Controller" },
	{ 0x03, 0x00, 0x01, "Display Controller", "8512-Compatible Controller" },
	{ 0x03, 0x01, 0x00, "Display Controller", "XGA Controller" },
	{ 0x03, 0x02, 0x00, "Display Controller", "3D Controller (Not VGA-Compatible)" },
	{ 0x03, 0x80, 0x00, "Display Controller", "Other" },

	{ 0x04, 0x00, 0x00, "Multimedia Device", "Video" },
	{ 0x04, 0x01, 0x00, "Multimedia Device", "Audio" },
	{ 0x04, 0x02, 0x00, "Multimedia Device", "Computer Telephony Device" },
	{ 0x04, 0x80, 0x00, "Multimedia Device", "Other" },

	{ 0x05, 0x00, 0x00, "Memory Controller", "RAM Controller" },
	{ 0x05, 0x01, 0x00, "Memory Controller", "Flash Controller" },
	{ 0x05, 0x80, 0x00, "Memory Controller", "Other" },

	{ 0x06, 0x00, 0x00, "Bridge Device", "Host-PCI" },
	{ 0x06, 0x01, 0x00, "Bridge Device", "ISA Bridge" },
	{ 0x06, 0x02, 0x00, "Bridge Device", "EISA Bridge" },
	{ 0x06, 0x03, 0x00, "Bridge Device", "MCA Bridge" },
	{ 0x06, 0x04, 0x00, "Bridge Device", "PCI-to-PCI Bridge" },
	{ 0x06, 0x04, 0x01, "Bridge Device", "PCI-to-PCI Bridge (Subtractive Decode)" },
	{ 0x06, 0x05, 0x00, "Bridge Device", "PCMCIA Bridge" },
	{ 0x06, 0x06, 0x00, "Bridge Device", "NuBus Bridge" },
	{ 0x06, 0x07, 0x00, "Bridge Device", "CardBus Bridge" },
	{ 0x06, 0x08, 0x00, "Bridge Device", "RACEway Bridge" },
	{ 0x06, 0x09, 0x00, "Bridge Device", "PCI-to-PCI Bridge (Primary, Semi-Transparent)" },
	{ 0x06, 0x09, 0x01, "Bridge Device", "PCI-to-PCI Bridge (Secondary, Semi-Transparent)" },
	{ 0x06, 0x0A, 0x00, "Bridge Device", "InfiniBrand-to-PCI Host Bridge" },
	{ 0x06, 0x80, 0x00, "Bridge Device", "Other" },

	{ 0x07, 0x00, 0x00, "Simple Communications Controller", "Generic XT-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x01, "Simple Communications Controller", "16450-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x02, "Simple Communications Controller", "16550-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x03, "Simple Communications Controller", "16650-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x04, "Simple Communications Controller", "16750-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x05, "Simple Communications Controller", "16850-Compatible Serial Controller" },
	{ 0x07, 0x00, 0x06, "Simple Communications Controller", "16950-Compatible Serial Controller" },
	{ 0x07, 0x01, 0x00, "Simple Communications Controller", "Parallel Port" },
	{ 0x07, 0x01, 0x01, "Simple Communications Controller", "Bi-Directional Parallel Port" },
	{ 0x07, 0x01, 0x02, "Simple Communications Controller", "ECP 1.X Compliant Parallel Port" },
	{ 0x07, 0x01, 0x03, "Simple Communications Controller", "IEEE 1284 Controller" },
	{ 0x07, 0x01, 0xFE, "Simple Communications Controller", "IEEE 1284 Target Device" },
	{ 0x07, 0x02, 0x00, "Simple Communications Controller", "Multiport Serial Controller" },
	{ 0x07, 0x03, 0x00, "Simple Communications Controller", "Generic Modem" },
	{ 0x07, 0x03, 0x01, "Simple Communications Controller", "Hayes (16450-Compatible Interface)" },
	{ 0x07, 0x03, 0x02, "Simple Communications Controller", "Hayes (16550-Compatible Interface)" },
	{ 0x07, 0x03, 0x03, "Simple Communications Controller", "Hayes (16650-Compatible Interface)" },
	{ 0x07, 0x03, 0x04, "Simple Communications Controller", "Hayes (16750-Compatible Interface)" },
	{ 0x07, 0x04, 0x00, "Simple Communications Controller", "IEEE 488.1/2 (GPIB) Controller" },
	{ 0x07, 0x05, 0x00, "Simple Communications Controller", "Smart Card" },
	{ 0x07, 0x80, 0x02, "Simple Communications Controller", "Other" },

	{ 0x08, 0x00, 0x00, "Base Systems Peripheral", "Generic 8259 PIC" },
	{ 0x08, 0x00, 0x01, "Base Systems Peripheral", "ISA PIC" },
	{ 0x08, 0x00, 0x02, "Base Systems Peripheral", "EISA PIC" },
	{ 0x08, 0x00, 0x10, "Base Systems Peripheral", "I/O APIC Interrupt Controller" },
	{ 0x08, 0x00, 0x20, "Base Systems Peripheral", "I/O(x) APIC Interrupt Controller" },
	{ 0x08, 0x01, 0x00, "Base Systems Peripheral", "Generic 8237 DMA Controller" },
	{ 0x08, 0x01, 0x01, "Base Systems Peripheral", "ISA DMA Controller" },
	{ 0x08, 0x01, 0x02, "Base Systems Peripheral", "EISA DMA Controller" },
	{ 0x08, 0x02, 0x00, "Base Systems Peripheral", "Generic 8254 System Timer" },
	{ 0x08, 0x02, 0x01, "Base Systems Peripheral", "ISA System Timer" },
	{ 0x08, 0x02, 0x02, "Base Systems Peripheral", "EISA System Timer" },
	{ 0x08, 0x03, 0x00, "Base Systems Peripheral", "Generic RTC Controller" },
	{ 0x08, 0x03, 0x01, "Base Systems Peripheral", "ISA RTC Controller" },
	{ 0x08, 0x04, 0x00, "Base Systems Peripheral", "Generic PCI Hot-Plug Controller" },
	{ 0x08, 0x80, 0x00, "Base Systems Peripheral", "Other" },

	{ 0x09, 0x00, 0x00, "Input Device", "Keyboard" },
	{ 0x09, 0x01, 0x00, "Input Device", "Digitizer (Pen)" },
	{ 0x09, 0x02, 0x00, "Input Device", "Mouse" },
	{ 0x09, 0x03, 0x00, "Input Device", "Scanner Controller" },
	{ 0x09, 0x04, 0x00, "Input Device", "Gameport Controller (Generic)" },
	{ 0x09, 0x04, 0x01, "Input Device", "Gameport Controller (Legacy)" },
	{ 0x09, 0x80, 0x00, "Input Device", "Other" },

	{ 0x0A, 0x00, 0x00, "Docking Station", "Generic" },
	{ 0x0A, 0x80, 0x00, "Docking Station", "Other" },

	{ 0x0B, 0x00, 0x00, "Processor", "i386 Processor" },
	{ 0x0B, 0x01, 0x00, "Processor", "i486 Processor" },
	{ 0x0B, 0x02, 0x00, "Processor", "Pentium Processor" },
	{ 0x0B, 0x10, 0x00, "Processor", "Alpha Processor" },
	{ 0x0B, 0x20, 0x00, "Processor", "Power PC Processor" },
	{ 0x0B, 0x30, 0x00, "Processor", "MIPS Processor" },
	{ 0x0B, 0x40, 0x00, "Processor", "Co-processor" },

	{ 0x0C, 0x00, 0x00, "Serial Bus Controller", "IEEE 1394 Controller (FireWire)" },
	{ 0x0C, 0x00, 0x10, "Serial Bus Controller", "IEEE 1394 Controller (1394 OpenHCI Spec.)" },
	{ 0x0C, 0x01, 0x00, "Serial Bus Controller", "ACCESS.bus" },
	{ 0x0C, 0x02, 0x00, "Serial Bus Controller", "SSA" },
	{ 0x0C, 0x03, 0x00, "Serial Bus Controller", "USB (Universal Host Controller Spec.)" },
	{ 0x0C, 0x03, 0x10, "Serial Bus Controller", "USB (Open Host Controller Spec.)" },
	{ 0x0C, 0x03, 0x20, "Serial Bus Controller", "USB2 Host Controller (Intel Enhanced Host Controller Interface)" },
	{ 0x0C, 0x03, 0x30, "Serial Bus Controller", "USB3 XHCI Controller" },
	{ 0x0C, 0x03, 0x80, "Serial Bus Controller", "Unspecified USB Controller" },
	{ 0x0C, 0x03, 0xFE, "Serial Bus Controller", "USB (Not Host Controller)" },
	{ 0x0C, 0x04, 0x00, "Serial Bus Controller", "Fibre Channel" },
	{ 0x0C, 0x05, 0x00, "Serial Bus Controller", "SMBus" },
	{ 0x0C, 0x06, 0x00, "Serial Bus Controller", "InfiniBand" },
	{ 0x0C, 0x07, 0x00, "Serial Bus Controller", "IPMI SMIC Interface" },
	{ 0x0C, 0x07, 0x01, "Serial Bus Controller", "IPMI Kybd Controller Style Interface" },
	{ 0x0C, 0x07, 0x02, "Serial Bus Controller", "IPMI Block Transfer Interface" },
	{ 0x0C, 0x08, 0x00, "Serial Bus Controller", "SERCOS Interface Standard (IEC 61491)" },
	{ 0x0C, 0x09, 0x00, "Serial Bus Controller", "CANbus" },

	{ 0x0D, 0x00, 0x00, "Wireless Controller", "iRDA Compatible Controller" },
	{ 0x0D, 0x01, 0x00, "Wireless Controller", "Consumer IR Controller" },
	{ 0x0D, 0x10, 0x00, "Wireless Controller", "RF Controller" },
	{ 0x0D, 0x11, 0x00, "Wireless Controller", "Bluetooth Controller" },
	{ 0x0D, 0x12, 0x00, "Wireless Controller", "Broadband Controller" },
	{ 0x0D, 0x20, 0x00, "Wireless Controller", "Ethernet Controller (802.11a)" },
	{ 0x0D, 0x21, 0x00, "Wireless Controller", "Ethernet Controller (802.11b)" },
	{ 0x0D, 0x80, 0x00, "Wireless Controller", "Other" },

	{ 0x0E, 0x00, 0x00, "Intelligent I/O Controller", "Message FIFO" },
	{ 0x0E, 0x00, 0x01, "Intelligent I/O Controller", "I20 Architecture" },

	{ 0x0F, 0x01, 0x00, "Satellite Communication Controller", "TV Controller" },
	{ 0x0F, 0x02, 0x00, "Satellite Communication Controller", "Audio Controller" },
	{ 0x0F, 0x03, 0x00, "Satellite Communication Controller", "Voice Controller" },
	{ 0x0F, 0x04, 0x00, "Satellite Communication Controller", "Data Controller" },

	{ 0x10, 0x00, 0x00, "Encryption/Decryption Controller", "Network and Computing" },
	{ 0x10, 0x10, 0x00, "Encryption/Decryption Controller", "Entertainment" },
	{ 0x10, 0x80, 0x00, "Encryption/Decryption Controller", "Other" },

	{ 0x11, 0x00, 0x00, "Data Acq./Signal Proc. Controllers", "DPIO Modules" },
	{ 0x11, 0x01, 0x00, "Data Acq./Signal Proc. Controllers", "Performance Counters" },
	{ 0x11, 0x10, 0x00, "Data Acq./Signal Proc. Controllers", "Communications Sync. + Time and Frequency Test/Measurement" },
	{ 0x11, 0x20, 0x00, "Data Acq./Signal Proc. Controllers", "Management Card" },
	{ 0x11, 0x80, 0x00, "Data Acq./Signal Proc. Controllers", "Other" },

	{ 0xFF, 0x00, 0x00, "Unknown", "Unknown device" }
};

/**
 *
 */
int pci_list() {

	// read how many devices there are
	g_kernquery_pci_count_out out;

	g_kernquery_status countstatus = g_kernquery(G_KERNQUERY_PCI_COUNT, 0, (uint8_t*) &out);
	if (countstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
		fprintf(stderr, "failed to query the kernel for the number of PCI devices (code %i)\n", countstatus);
		return 1;
	}

	// list devices
	uint32_t deviceCount = out.count;
	printf("Found %i PCI devices\n", deviceCount);

	// tables top line
	printf("\xDA");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC2");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xBF");

	printf("\xB3 B. \xB3 S. \xB3 F. \xB3 Ven. \xB3 Dev. \xB3 Class                  \xB3 Subclass               \xB3");

	printf("\xC3");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC5");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xB4");

	for (uint32_t pos = 0; pos < deviceCount; pos++) {

		// query device at position
		g_kernquery_pci_get_in in;
		in.position = pos;

		g_kernquery_pci_get_out out;

		g_kernquery_status getstatus = g_kernquery(G_KERNQUERY_PCI_GET, (uint8_t*) &in, (uint8_t*) &out);
		if (getstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
			fprintf(stderr, "failed to query the kernel for PCI devices %i (code %i)", pos, getstatus);
			return 1;
		}

		// find the matching entry
		PCIClassMapping* matchingEntry = 0;
		PCIClassMapping* matchingNoProgifEntry = 0;
		for (int i = 0; i < sizeof(PCI_CLASSCODE_MAPPINGS) / sizeof(PCIClassMapping); i++) {
			PCIClassMapping* table = &PCI_CLASSCODE_MAPPINGS[i];
			if (table->classCode == out.classCode && table->subclassCode == out.subclassCode && table->progIf == out.progIf) {
				matchingEntry = table;
				break;

			} else if(table->classCode == out.classCode && table->subclassCode == out.subclassCode) {
				matchingNoProgifEntry = table;
				break;
			}
		}

		const char* className = "?";
		const char* subclassName = "?";
		if(matchingEntry != 0) {
			className = matchingEntry->className;
			subclassName = matchingEntry->subclassName;

		} else if(matchingNoProgifEntry) {
			className = matchingNoProgifEntry->className;
			subclassName = matchingNoProgifEntry->subclassName;
		}
		printf("\xB3 %02x \xB3 %02x \xB3 %02x \xB3 %04x \xB3 %04x \xB3 %.22s \xB3 %.22s \xB3", out.bus, out.slot, out.function, out.vendorId,
					out.deviceId, className, subclassName);
	}

	// bottom line
	printf("\xC0");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xC1");
	printf("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");
	printf("\xD9");
}
