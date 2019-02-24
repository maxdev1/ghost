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

#include "shared/system/serial_port.hpp"
#include "shared/system/io_port.hpp"

void serialPortInitialize(uint16_t port, bool interruptsEnabled)
{
	// Disable the interrupts
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_INTERRUPT_ENABLE, 0x00);

	// Set DLAB
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_LINE_CONTROL, 0x80);

	// Divisor to 3, baud 38400
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_DIVISOR_LEAST, 0x03); // Low byte
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_DIVISOR_MOST, 0x00); // High byte

	// Set line control to 0x03. Bits:
	// 0: 1  Charlen 8
	// 1: 1  ^
	// 2: 0  one stop bit
	// 3: 0  no parity
	// 4: 0  ^
	// 5: 0  ^
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_LINE_CONTROL, 0x03);

	if (interruptsEnabled)
	{
		// Enable FIFO
		ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_INT_FIFO, 0xC7);

		// Enable IRQs with RTS/DSR set
		ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_MODEM_CONTROL, 0x0B);

		// Enable the interrupts
		// 0: 1  Receiver has data interrupt on
		// 1: 1  Transmitter empty interrupt on
		// 2: 0  Error/break interrupt off
		// 3: 0  Status change interrupt off
		ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_INTERRUPT_ENABLE, 0x02);
	}
	else
	{
		// Turn off FIFO
		ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_INT_FIFO, 0x00);

		// Loopback off, no IRQs, not RTS/DTR
		ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_MODEM_CONTROL, 0x00);
	}
}

uint8_t serialPortRead(uint16_t port)
{
	// Wait byte available
	while ((ioPortReadByte(port + G_SERIAL_PORT_OFFSET_LINE_STATUS) & 0x01) == 0)
	{
	}

	// Receive byte
	return ioPortReadByte(port + G_SERIAL_PORT_OFFSET_DATA_REGISTER);
}

void serialPortWrite(uint16_t port, uint8_t value)
{
	// Wait until ready
	while ((ioPortReadByte(port + G_SERIAL_PORT_OFFSET_LINE_STATUS) & 0x20) == 0)
	{
	}

	// Write byte
	ioPortWriteByte(port + G_SERIAL_PORT_OFFSET_DATA_REGISTER, value);
}

