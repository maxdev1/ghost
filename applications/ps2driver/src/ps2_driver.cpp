#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#include "ps2_driver.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ghost.h>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <ghostuser/utils/utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>

/**
 *
 */
uint8_t mouse_packet_number = 0;
uint8_t mouse_packet_buffer[3];
int32_t mouse_position_x = 0;
int32_t mouse_position_y = 0;

/**
 * Receiver task ids and transaction ids for dispatching incoming packets.
 */
uint32_t mouse_receiver_tid;
uint32_t mouse_receiver_transaction;
uint32_t keyboard_receiver_tid;
uint32_t keyboard_receiver_transaction;

/**
 *
 */
uint64_t packets_count = 0;

/**
 *
 */
int main() {

	// register
	if (!g_task_register_id(G_PS2_DRIVER_IDENTIFIER)) {
		g_logger::log("failed to register as '%s'",
				(char*) G_PS2_DRIVER_IDENTIFIER);
	}

	// initialize mouse
	initialize_mouse();

	// start receiver threads
	register_operation_mode();

	// wait for control requests
	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_register_request);
	uint8_t buf[buflen];
	g_tid tid = g_get_tid();
	while (true) {
		g_message_receive_status stat = g_receive_message(buf, buflen);

		if (stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			g_message_header* mes = (g_message_header*) buf;
			g_ps2_register_request* req =
					(g_ps2_register_request*) G_MESSAGE_CONTENT(buf);

			if (req->command == G_PS2_COMMAND_REGISTER_KEYBOARD) {
				keyboard_receiver_tid = mes->sender;
				keyboard_receiver_transaction = mes->transaction;

			} else if (req->command == G_PS2_COMMAND_REGISTER_MOUSE) {
				mouse_receiver_tid = mes->sender;
				mouse_receiver_transaction = mes->transaction;

			} else {
				g_logger::log("received unknown command: %i", req->command);
			}
		}
	}
}

/**
 *
 */
void initialize_mouse() {

	uint8_t status;

	// empty input buffer
	while (g_utils::inportByte(0x64) & 0x01) {
		g_utils::inportByte(0x60);
	}

	// activate mouse device
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x64, 0xA8);

	// get commando-byte, set bit 1 (enables IRQ12), send back
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x64, 0x20);

	wait_for_buffer(PS2_IN);
	status = g_utils::inportByte(0x60) | (1 << 1);

	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x64, 0x60);
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x60, status);

	// send set-default-settings command to mouse
	write_to_mouse(0xF6);

	wait_for_buffer(PS2_IN);
	status = g_utils::inportByte(0x60);
	if (status != 0xFA) {
		g_logger::log("mouse did not ack set-default-settings command");
		return;
	}

	// enable the mouse
	write_to_mouse(0xF4);

	wait_for_buffer(PS2_IN);
	status = g_utils::inportByte(0x60);
	if (status != 0xFA) {
		g_logger::log("mouse did not ack enable-mouse command");
		return;
	}
}

/**
 *
 */
void handle_mouse_data(uint8_t b) {

	switch (mouse_packet_number) {
	case 0:
		mouse_packet_buffer[0] = b;

		// flags byte must have bit 3 set
		if ((b & (1 << 3)) == 0) {
			mouse_packet_number = 0; // otherwise restart the cycle
		} else {
			mouse_packet_number = 1;
		}
		break;

	case 1:
		mouse_packet_buffer[1] = b;
		mouse_packet_number = 2;
		break;

	case 2:
		mouse_packet_buffer[2] = b;

		int8_t flags = mouse_packet_buffer[0];
		uint8_t valX = mouse_packet_buffer[1];
		uint8_t valY = mouse_packet_buffer[2];

		if ((flags & (1 << 6)) || (flags & (1 << 7))) {
			// ignore overflowing values

		} else {
			int16_t offX = valX - ((flags << 4) & 0x100);
			int16_t offY = valY - ((flags << 3) & 0x100);

			g_ps2_mouse_packet packet;
			packet.x = offX;
			packet.y = offY;
			packet.flags = flags;
			g_send_message_t(mouse_receiver_tid, &packet,
					sizeof(g_ps2_mouse_packet), mouse_receiver_transaction);
		}

		mouse_packet_number = 0;
		break;
	}
}

/**
 *
 */
void handle_keyboard_data(uint8_t b) {

	g_ps2_keyboard_packet packet;
	packet.scancode = b;

	// ESC is the test key
	if (b == 1) {
		g_test(1);
	}

	g_send_message_t(keyboard_receiver_tid, &packet,
			sizeof(g_ps2_keyboard_packet), keyboard_receiver_transaction);
}

/**
 *
 */
void wait_for_buffer(ps2_buffer_t buffer) {

	uint32_t timeout = 100000;

	if (buffer == PS2_OUT) {
		while (timeout--) {
			if ((g_utils::inportByte(0x64) & 2) == 0) {
				return;
			}
		}

	} else if (buffer == PS2_IN) {
		while (timeout--) {
			if ((g_utils::inportByte(0x64) & 1) == 1) {
				return;
			}
		}
	}
}

/**
 *
 */
void write_to_mouse(uint8_t value) {

	// tell the controller that we want to write to the mouse
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x64, 0xD4);

	// send the value to the mouse
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x60, value);
}

