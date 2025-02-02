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

#ifndef __VMSVGADRIVER__
#define __VMSVGADRIVER__

#include "svga_reg.h"

#include <libvmsvgadriver/vmsvgadriver.hpp>
#include <ghost/memory.h>

struct svga_device_t
{
    uint32_t versionId = SVGA_ID_2;
    uint32_t ioBase;

    uint32_t vramSize;

    g_physical_address fbPhysical;
    uint32_t fbSize;

    g_physical_address fifoPhysical;
    uint32_t fifoSize;
    uint32_t* fifo;
};


/**
 *
 */
void vmsvgaDriverReceiveMessages();

#endif
