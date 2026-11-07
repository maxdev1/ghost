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

#ifndef GHOST_API_SYSTEM_IRQ
#define GHOST_API_SYSTEM_IRQ

#include "../common.h"
#include "../stdint.h"

__BEGIN_C

/**
 * Selects how g_irq_bind interprets its line argument.
 *
 * ISA takes an ISA IRQ number. The kernel applies firmware overrides for the
 * pin, polarity and trigger mode. The defaults are active-high and edge-triggered.
 * PCI_INTX takes the global system interrupt (GSI) routed to the PCI function.
 * ACPI takes a firmware GSI or the SCI number from the FADT. For the SCI, the
 * kernel applies its MADT override. It requires a level-triggered route and
 * defaults to active-low polarity. Only the ACPI service may bind this source.
 */
typedef uint8_t g_irq_source;
#define G_IRQ_SOURCE_ISA ((g_irq_source) 1)
#define G_IRQ_SOURCE_PCI_INTX ((g_irq_source) 2)
#define G_IRQ_SOURCE_ACPI ((g_irq_source) 3)

/**
 * Identifies one binding owned by the driver's process. Only the thread that
 * bound the IRQ may wait on it. Use G_IRQ_HANDLE_NONE for an empty handle.
 */
typedef uint64_t g_irq_handle;
#define G_IRQ_HANDLE_NONE ((g_irq_handle) 0)

/**
 * G_IRQ_SUCCESSFUL means a control operation succeeded. A wait instead
 * reports G_IRQ_DELIVERED, G_IRQ_TIMED_OUT or G_IRQ_RELEASED.
 * G_IRQ_BUSY means the route or its vector is occupied or still draining.
 * G_IRQ_INVALID_ARGUMENT indicates an invalid argument such as an empty handle.
 * G_IRQ_NOT_PERMITTED indicates a privilege or ownership failure.
 * G_IRQ_NOT_SUPPORTED indicates the requested route cannot be provided.
 */
typedef uint8_t g_irq_status;
#define G_IRQ_SUCCESSFUL ((g_irq_status) 0)
#define G_IRQ_DELIVERED ((g_irq_status) 1)
#define G_IRQ_TIMED_OUT ((g_irq_status) 2)
#define G_IRQ_RELEASED ((g_irq_status) 3)
#define G_IRQ_BUSY ((g_irq_status) 4)
#define G_IRQ_INVALID_ARGUMENT ((g_irq_status) 5)
#define G_IRQ_NOT_PERMITTED ((g_irq_status) 6)
#define G_IRQ_NOT_SUPPORTED ((g_irq_status) 7)

__END_C

#endif
