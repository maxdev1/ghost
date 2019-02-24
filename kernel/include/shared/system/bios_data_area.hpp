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

#ifndef __BIOS_DATA_AREA__
#define __BIOS_DATA_AREA

#include "ghost/stdint.h"

/**
 * COM port information within the BIOS data area
 */
struct g_com_port_information
{
	uint16_t com1;
	uint16_t com2;
	uint16_t com3;
	uint16_t com4;
}__attribute__((packed));

/**
 * LPT information
 */
struct g_lpt_information
{
	uint16_t lpt1;
	uint16_t lpt2;
	uint16_t lpt3;
}__attribute__((packed));

/**
 * Bios data area structure
 */
struct g_bios_data_area
{
	g_com_port_information comPortInfo;
	g_lpt_information lptInfo;

	uint16_t ebdaShiftedAddr; // must be left-shifted by 4

	/* Other data is currently not used */
}__attribute__((packed));

/**
 * Pointer to the bios data area
 */
extern g_bios_data_area *biosDataArea;

#endif
