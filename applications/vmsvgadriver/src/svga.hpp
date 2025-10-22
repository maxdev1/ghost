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

#ifndef __SVGA__
#define __SVGA__

#include "svga_reg.h"
#include <ghost/memory.h>

struct svga_device_t
{
	uint32_t versionId = SVGA_ID_2;
	g_address ioBase = 0;
	g_size vramSize = 0;

	struct
	{
		g_physical_address physical = 0;
		uint32_t size = 0;
		uint32_t* mapped = nullptr;
	} fb;

	struct
	{
		g_physical_address physical = 0;
		uint32_t size = 0;
		uint32_t* mapped = nullptr;

		uint32_t reservedSize = 0;
	} fifo;
};

bool svgaInitializeDevice();
bool svgaIdentifyVersion();
uint32_t svgaReadReg(uint32_t index);
void svgaWriteReg(uint32_t index, uint32_t value);
void svgaSetMode(uint32_t width, uint32_t height, uint32_t bpp);

uint32_t* svgaGetFb();
uint32_t svgaGetFbSize();
bool svgaFifoHasCapability(int cap);

void* svgaFifoReserveSpace(uint32_t size);
void* svgaFifoReserveCommand(uint32_t type, uint32_t bytes);
void svgaFifoCommit(uint32_t bytes);
void svgaFifoCommitReserved();
void svgaUpdate(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#endif
