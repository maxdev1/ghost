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

#include "libps2/ps2.hpp"
#include "ps2_intern.hpp"

#include <ghost.h>
#include <stdio.h>

uint8_t mousePacketNumber = 0;
uint8_t mousePacketBuffer[4];
bool intelliMouseMode = false; // 4-byte sequences

g_fd keyIrqIn;
g_fd mouseIrqIn;

uint32_t packetCount = 0;

void (*registeredMouseCallback)(int16_t, int16_t, uint8_t, int8_t);
void (*registeredKeyboardCallback)(uint8_t);

void ps2HandlePacket();

ps2_status_t ps2Initialize(void (*mouseCallback)(int16_t, int16_t, uint8_t, int8_t),
                           void (*keyboardCallback)(uint8_t))
{

	registeredMouseCallback = mouseCallback;
	registeredKeyboardCallback = keyboardCallback;

	ps2_status_t status = ps2InitializeMouse();
	if(status != G_PS2_STATUS_SUCCESS)
	{
		return status;
	}

	g_irq_create_redirect(1, 1);
	g_irq_create_redirect(12, 12);

	g_create_task_a((void*) &ps2ReadKeyIrq, 0);
	g_create_task_a((void*) &ps2ReadMouseIrq, 0);
	return G_PS2_STATUS_SUCCESS;
}

void ps2ReadKeyIrq()
{
	g_task_register_id("libps2/read-key");
	for(;;)
	{
		g_await_irq_t(1, 1000);
		ps2CheckForData();
	}
}

void ps2ReadMouseIrq()
{
	g_task_register_id("libps2/read-mouse");
	for(;;)
	{
		g_await_irq_t(12, 1000);
		ps2CheckForData();
	}
}

void ps2CheckForData()
{
	uint8_t status;
	while(((status = g_io_port_read_byte(G_PS2_STATUS_PORT)) & 0x01) != 0)
	{
		uint8_t value = g_io_port_read_byte(G_PS2_DATA_PORT);

		if((status & 0x20) == 0)
		{
			if(registeredKeyboardCallback)
			{
				registeredKeyboardCallback(value);
			}
		}
		else
		{
			ps2HandleMouseData(value);
		}

		++packetCount;
	}
}

ps2_status_t ps2InitializeMouse()
{

	// empty input buffer
	while(g_io_port_read_byte(G_PS2_STATUS_PORT) & 0x01)
	{
		g_io_port_read_byte(G_PS2_DATA_PORT);
	}

	// activate mouse device
	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_STATUS_PORT, 0xA8);
	ps2WaitForBuffer(PS2_IN);
	g_io_port_read_byte(G_PS2_DATA_PORT);

	// get commando-byte, set bit 1 (enables IRQ12), send back
	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_STATUS_PORT, 0x20);

	ps2WaitForBuffer(PS2_IN);
	uint8_t status = (g_io_port_read_byte(G_PS2_DATA_PORT) | 0x02) & (~0x10);

	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_STATUS_PORT, 0x60);
	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_DATA_PORT, status);

	// send set-default-settings command to mouse
	if(ps2WriteToMouse(0xF6))
	{
		klog("error: mouse did not acknowledge setting defaults command");
		return G_PS2_STATUS_FAILED_INITIALIZE;
	}

	// enable the mouse
	if(ps2WriteToMouse(0xF4))
	{
		klog("mouse did not acknowledge enable-mouse command");
		return G_PS2_STATUS_FAILED_INITIALIZE;
	}

	// enable extended stuff for scrolling
	if(ps2WriteToMouse(0xF3) ||
	   ps2WriteToMouse(200) ||
	   ps2WriteToMouse(0xF3) ||
	   ps2WriteToMouse(100) ||
	   ps2WriteToMouse(0xF3) ||
	   ps2WriteToMouse(80))
	{
		intelliMouseMode = false;
		klog("failed to enable intellimouse mode");
	}
	else
	{
		intelliMouseMode = true;
	}

	return G_PS2_STATUS_SUCCESS;
}

void ps2HandleMouseData(uint8_t value)
{
	static uint64_t lastPacketTime = 0;
	uint64_t now = g_millis();

	// Reset packet state if too much time has passed
	if(mousePacketNumber > 0 && (now - lastPacketTime > 50))
		mousePacketNumber = 0;

	lastPacketTime = now;

	if(mousePacketNumber == 0)
	{
		mousePacketBuffer[0] = value;
		// The first byte must always have the 4th bit (0x08) set
		if((value & 0x08) == 0)
		{
			mousePacketNumber = 0; // wait for a valid starting byte
		}
		else
		{
			mousePacketNumber = 1;
		}
	}
	else if(mousePacketNumber == 1)
	{
		mousePacketBuffer[1] = value;
		mousePacketNumber = 2;
	}
	else if(mousePacketNumber == 2)
	{
		mousePacketBuffer[2] = value;
		if(intelliMouseMode)
		{
			mousePacketNumber = 3;
		}
		else
		{
			mousePacketNumber = 0;
			ps2HandlePacket();
		}
	}
	else if(mousePacketNumber == 3) // IntelliMouse only
	{
		mousePacketBuffer[3] = value;
		mousePacketNumber = 0;
		ps2HandlePacket();
	}
}


void ps2HandlePacket()
{
	int8_t flags = mousePacketBuffer[0];
	uint8_t valX = mousePacketBuffer[1];
	uint8_t valY = mousePacketBuffer[2];
	int8_t scroll = intelliMouseMode ? (int8_t) mousePacketBuffer[3] : 0;

	int16_t offX = (valX | ((flags & 0x10) ? 0xFF00 : 0));
	int16_t offY = (valY | ((flags & 0x20) ? 0xFF00 : 0));
	if((flags & (1 << 6)) || (flags & (1 << 7)))
	{
		offX = 0;
		offY = 0;
	}

	if(registeredMouseCallback)
		registeredMouseCallback(offX, -offY, flags, scroll);
}

void ps2WaitForBuffer(ps2_buffer_t buffer)
{

	uint8_t requiredBit;
	uint8_t requiredValue;

	if(buffer == PS2_OUT)
	{
		requiredBit = 0x02;
		requiredValue = 0;
	}
	else if(buffer == PS2_IN)
	{
		requiredBit = 0x01;
		requiredValue = 1;
	}

	int timeout = 10000;
	while(timeout--)
	{
		if((g_io_port_read_byte(G_PS2_STATUS_PORT) & requiredBit) == requiredValue)
		{
			return;
		}
		asm volatile("pause");
	}
}

int ps2WriteToMouse(uint8_t value)
{

	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_STATUS_PORT, 0xD4);

	ps2WaitForBuffer(PS2_OUT);
	g_io_port_write_byte(G_PS2_DATA_PORT, value);

	ps2WaitForBuffer(PS2_IN);
	if(g_io_port_read_byte(G_PS2_DATA_PORT) != 0xFA)
	{
		return 1;
	}
	return 0;
}
