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

#ifndef __KERNEL_MEMORY_PAGING_SHOOTDOWN__
#define __KERNEL_MEMORY_PAGING_SHOOTDOWN__

#include <ghost/memory/types.h>

/**
 * Called after every boot CPU has installed its IDT, TSS and local APIC.
 */
void pagingInitializeSmp();

/**
 * Completes invalidation on every relevant CPU before an old frame can be reused.
 */
void pagingSynchronizePage(g_virtual_address address);

/**
 * Global ranges use explicit invalidations on every CPU, regardless of CR3.
 */
void pagingSynchronizePages(g_virtual_address address, uint32_t pages, bool global = false);

/**
 * Dedicated IPI entry: performs no allocation, locking, output or scheduling.
 */
bool pagingHandleShootdownIpi();

/**
 * Services this CPU's mailbox at kernel entry or while waiting for a lock.
 * Preserves interrupt state and performs no allocation, locking or output.
 */
void pagingPollShootdown();

#endif
