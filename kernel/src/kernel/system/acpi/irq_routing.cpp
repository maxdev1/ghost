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

#include "kernel/system/acpi/irq_routing.hpp"
#include "kernel/system/acpi/madt.hpp"

struct g_acpi_isa_irq
{
	uint32_t gsi;
	uint16_t flags;
};

static g_acpi_isa_irq acpiIsaRoutes[16];
static uint8_t acpiIrqExclusions[96];
static uint32_t acpiSciLine;

static constexpr uint8_t G_ACPI_IRQ_PCI_MASK = 1u << G_IRQ_SOURCE_PCI_INTX;
static constexpr uint8_t G_ACPI_IRQ_ISA_MASK = 1u << G_IRQ_SOURCE_ISA;
static constexpr uint8_t G_ACPI_IRQ_ALL_MASK = G_ACPI_IRQ_PCI_MASK | G_ACPI_IRQ_ISA_MASK | (1u << G_IRQ_SOURCE_ACPI);

static void acpiIrqExclude(uint32_t gsi, uint8_t sources)
{
	if(gsi < sizeof(acpiIrqExclusions))
		acpiIrqExclusions[gsi] |= sources;
}

/**
 * Keep PCI drivers from claiming traditional PC/AT lines before their ISA
 * drivers start. Protect both the original line and its firmware-mapped GSI.
 * The system timer is reserved against all driver claims.
 */
static void acpiIrqExcludeFixedIsaLines()
{
	constexpr uint8_t fixed[] = {
			0, // Programmable interval timer
			1, // Keyboard
			2, // 8259 PIC cascade
			3, // Serial ports COM2 and COM4
			4, // Serial ports COM1 and COM3
			6, // Floppy disk controller
			8, // Real-time clock
			12, // PS/2 mouse
			13, // x87 coprocessor error
			14, // Primary IDE channel
			15, // Secondary IDE channel
	};

	for(auto line: fixed)
	{
		acpiIrqExclude(line, G_ACPI_IRQ_PCI_MASK);
		acpiIrqExclude(acpiIsaRoutes[line].gsi, G_ACPI_IRQ_PCI_MASK);
	}

	acpiIrqExclude(0, G_ACPI_IRQ_ALL_MASK);
	acpiIrqExclude(acpiIsaRoutes[0].gsi, G_ACPI_IRQ_ALL_MASK);
}

static void acpiIrqExcludeSci(const g_acpi_table_header* fadt)
{
	constexpr uint32_t sciIntOffset = 46;
	acpiSciLine = 9;
	if(fadt && fadt->length >= sciIntOffset + sizeof(uint16_t))
	{
		auto bytes = (const uint8_t*) fadt;
		acpiSciLine = bytes[sciIntOffset] | (uint32_t(bytes[sciIntOffset + 1]) << 8);
	}

	acpiIrqExclude(acpiSciLine, G_ACPI_IRQ_PCI_MASK | G_ACPI_IRQ_ISA_MASK);
	if(acpiSciLine < 16)
		acpiIrqExclude(acpiIsaRoutes[acpiSciLine].gsi, G_ACPI_IRQ_PCI_MASK | G_ACPI_IRQ_ISA_MASK);
}

void acpiIrqConfigure(const g_acpi_table_header* fadt, const g_acpi_table_header* madt)
{
	for(auto& exclusions: acpiIrqExclusions)
		exclusions = 0;
	for(uint32_t line = 0; line < 16; ++line)
		acpiIsaRoutes[line] = {line, 0};

	if(madt && madt->length >= sizeof(g_madt_header))
	{
		auto bytes = (const uint8_t*) madt;
		for(uint32_t offset = sizeof(g_madt_header); madt->length - offset >= sizeof(g_madt_entry_header);)
		{
			auto entry = (const g_madt_entry_header*) (bytes + offset);
			if(entry->recordLength < sizeof(g_madt_entry_header) || entry->recordLength > madt->length - offset)
				break;

			if(entry->deviceType == 2 && entry->recordLength >= sizeof(g_madt_interrupt_src_override_entry))
			{
				auto route = (const g_madt_interrupt_src_override_entry*) entry;
				if(!route->busSource && route->irqSource < 16)
					acpiIsaRoutes[route->irqSource] = {route->globalSystemInterrupt, route->flags};
			}

			else if(entry->deviceType == 3 && entry->recordLength >= 8)
			{
				auto nmi = bytes + offset + 4;
				uint32_t gsi = nmi[0] | (uint32_t(nmi[1]) << 8) | (uint32_t(nmi[2]) << 16) | (uint32_t(nmi[3]) << 24);
				acpiIrqExclude(gsi, G_ACPI_IRQ_ALL_MASK);
			}

			offset += entry->recordLength;
		}
	}

	acpiIrqExcludeFixedIsaLines();
	acpiIrqExcludeSci(fadt);
}

bool acpiIrqResolveIsa(uint32_t line, uint32_t* gsi, bool* activeLow, bool* level)
{
	if(!line || line >= 16 || line == 2)
		return false;

	const auto& route = acpiIsaRoutes[line];
	uint16_t polarity = route.flags & 3;
	uint16_t trigger = (route.flags >> 2) & 3;

	if(polarity == 2 || trigger == 2)
		return false;

	*gsi = route.gsi;
	*activeLow = polarity == 3;
	*level = trigger == 3;
	return true;
}

bool acpiIrqResolveSci(uint32_t line, uint32_t* gsi, bool* activeLow)
{
	if(acpiSciLine >= 16)
		return true;

	const auto& sci = acpiIsaRoutes[acpiSciLine];
	if(line != acpiSciLine && line != sci.gsi)
		return true;

	uint16_t polarity = sci.flags & 3;
	uint16_t trigger = (sci.flags >> 2) & 3;
	if(polarity == 2 || (trigger != 0 && trigger != 3))
		return false;

	*gsi = sci.gsi;
	*activeLow = polarity != 1;
	return true;
}

bool acpiIrqAllows(g_irq_source source, uint32_t gsi)
{
	return gsi < sizeof(acpiIrqExclusions) && !(acpiIrqExclusions[gsi] & (1u << source));
}
