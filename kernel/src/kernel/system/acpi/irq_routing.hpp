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

#ifndef G_KERNEL_ACPI_IRQ_ROUTING
#define G_KERNEL_ACPI_IRQ_ROUTING

#include <ghost/system/irq.h>

struct g_acpi_table_header;

/**
 * Initializes ISA mappings from MADT overrides and reserves fixed ISA, NMI
 * and FADT SCI lines for their permitted sources. Missing tables leave
 * identity ISA mappings and SCI line 9. Call before querying routes.
 */
void acpiIrqConfigure(const g_acpi_table_header* fadt, const g_acpi_table_header* madt);

/**
 * Returns the GSI, polarity and trigger mode for an ISA IRQ. Without a MADT
 * override, delivery is active-high and edge-triggered on the same GSI.
 * Returns false for IRQ 0, IRQ 2, lines above 15 or reserved flag encodings.
 */
bool acpiIrqResolveIsa(uint32_t line, uint32_t* gsi, bool* activeLow, bool* level);

/**
 * Applies the MADT mapping and polarity when line matches the legacy SCI
 * line or its mapped GSI. Other lines leave the outputs unchanged; callers
 * must initialize them to the input GSI and active-low polarity.
 * Returns false for reserved polarity or overrides incompatible with
 * level triggering.
 */
bool acpiIrqResolveSci(uint32_t line, uint32_t* gsi, bool* activeLow);

/**
 * Returns whether the GSI is within the supported range and its configured
 * reservations permit claims from the given IRQ source.
 */
bool acpiIrqAllows(g_irq_source source, uint32_t gsi);

#endif
