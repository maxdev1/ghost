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

#include "kernel/system/interrupts/pic.hpp"
#include "shared/system/io_port.hpp"

void picSendEoi(uint8_t intr)
{
	/*
	 If the interrupt is an IRQ (> 32) and on the second PIC (> 8)
	 we have to tell the slave that the interrupt has ended too
	 */
	if(intr >= 0x20 + 0x08)
	{
		ioPortWriteByte(PIC2_COMMAND, 0x20);
	}

	ioPortWriteByte(PIC1_COMMAND, 0x20);
}

void picMaskIrq(uint8_t irq)
{

	uint16_t port;
	uint8_t value;

	if(irq < 8)
	{
		port = PIC1_DATA;
	} else
	{
		port = PIC2_DATA;
		irq -= 8;
	}

	value = ioPortReadByte(port);
	value |= (1 << irq);
	ioPortWriteByte(port, value);
}

void picUnmaskIrq(uint8_t irq)
{

	uint16_t port;
	uint8_t value;

	if(irq < 8)
	{
		port = PIC1_DATA;
	} else
	{
		port = PIC2_DATA;
		irq -= 8;
	}

	value = ioPortReadByte(port);
	value &= ~(1 << irq);
	ioPortWriteByte(port, value);

}

void picRemapIrqs()
{
	ioPortWriteByte(PIC1_COMMAND, 0x11);
	ioPortWriteByte(PIC2_COMMAND, 0x11);
	ioPortWriteByte(PIC1_DATA, 0x20); // IRQ0 - IRQ7 => interrupts 0x20-0x27
	ioPortWriteByte(PIC2_DATA, 0x28); // IRQ8 - IRQ15 => interrupts 0x28-0x2F
	ioPortWriteByte(PIC1_DATA, 0x4);
	ioPortWriteByte(PIC2_DATA, 0x2);
	ioPortWriteByte(PIC1_DATA, 0x1);
	ioPortWriteByte(PIC2_DATA, 0x1);
}

void picDisable()
{
	picRemapIrqs();
	picMaskAll();
}

void picUnmaskAll()
{
	ioPortWriteByte(PIC1_DATA, 0x0);
	ioPortWriteByte(PIC2_DATA, 0x0);
}

void picMaskAll()
{
	ioPortWriteByte(PIC1_DATA, 0xFF);
	ioPortWriteByte(PIC2_DATA, 0xFF);
}

