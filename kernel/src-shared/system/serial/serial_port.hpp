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

#ifndef GHOST_SHARED_SYSTEM_SERIAL_SERIALPORT
#define GHOST_SHARED_SYSTEM_SERIAL_SERIALPORT

#include "ghost/stdint.h"

/**
 * Constants for COM port communcation
 */
#define G_SERIAL_PORT_OFFSET_DATA_REGISTER			0  // without DLAB, register for receiving and writing
#define G_SERIAL_PORT_OFFSET_INTERRUPT_ENABLE		1  // without DLAB, interrupt enable register
#define G_SERIAL_PORT_OFFSET_DIVISOR_LEAST			0  // with DLAB, least significant divisor byte
#define G_SERIAL_PORT_OFFSET_DIVISOR_MOST			1  // with DLAB, most significant divisor byte
#define G_SERIAL_PORT_OFFSET_INT_FIFO				2  // interrupt identification and FIFO control register
#define G_SERIAL_PORT_OFFSET_LINE_CONTROL			3  // line control register
#define G_SERIAL_PORT_OFFSET_MODEM_CONTROL			4  // modem control register
#define G_SERIAL_PORT_OFFSET_LINE_STATUS			5  // line status register
#define G_SERIAL_PORT_OFFSET_MODEM_STATUS			6  // modem status register
#define G_SERIAL_PORT_OFFSET_SCRATCH				7  // scratch register

/**
 * Class for serial port access
 */
class g_serial_port {
public:
	static void initializePort(uint16_t port, bool interruptsEnabled);
	static uint8_t read(uint16_t port);
	static void write(uint16_t port, uint8_t value);
};

#endif
