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

#ifndef __KERNEL__
#define __KERNEL__

#include "ghost/types.h"
#include "shared/setup_information.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/logger/logger.hpp"

extern g_bitmap_page_allocator* kernelPhysicalAllocator;

/**
 * Main entry point of the kernel. The loader calls this function on the
 * bootstrap processor. The setup information structure contains information
 * about everything that the loader has prepared for the kernel.
 */
extern "C" void kernelMain(g_setup_information* setupInformation);

/**
 * Initializes the very basic components and then starts the BSP initialization sequence.
 */
void kernelInitialize(g_setup_information* setupInformation);

/**
 * Bootstrap processor (BSP) initialization sequence.
 */
void kernelRunBootstrapCore(g_physical_address initialPdPhys);

/**
 * Once the kernel is set up and all application processors are ready, this is the first thread
 * that can perform any work. This is necessary because we need an initial process so we can
 * open files and other resources.
 */
void kernelInitializationThread();

/**
 * This function is started by the SMP implementation.
 */
void kernelRunApplicationCore();

void kernelPanic(const char *msg, ...);

void kernelHalt();

#endif
