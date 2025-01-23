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

#ifndef __KERNEL_SYSTEM__
#define __KERNEL_SYSTEM__

#include <ghost/memory/types.h>

/**
 * Sets up all the basic system components that are required to initialize
 * higher level parts of the kernel. If multiple cores are available, the
 * initial physical page directory address is passed to their bootstrap code.
 */
void systemInitializeBsp(g_physical_address initialPdPhys);

/**
 * Sets up the remaining components which need local initialization on each core.
 */
void systemInitializeAp();

/**
 * Waits until all application cores were marked as ready.
 */
void systemWaitForApplicationCores();

/**
 * Wait until the system is marked as ready.
 */
void systemWaitForReady();

/**
 * Marks another application core as ready.
 */
void systemMarkApplicationCoreReady();

/**
 * Marks the system as ready, meaning that all vital system components are initialized
 * and tasking is ready to start.
 */
void systemMarkReady();

/**
 * @return true when the system bootstrap is finished on all processors.
 */
bool systemIsReady();

#endif
