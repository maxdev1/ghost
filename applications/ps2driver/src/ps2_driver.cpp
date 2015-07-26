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

#include <ps2_driver.hpp>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ghost.h>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <ghostuser/utils/utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>

uint8_t mouse_packet_number = 0;
uint8_t mouse_packet_buffer[3];
int32_t mouse_position_x = 0;
int32_t mouse_position_y = 0;

uint32_t mouse_receiver_tid;
uint32_t mouse_receiver_transaction;
uint32_t keyboard_receiver_tid;
uint32_t keyboard_receiver_transaction;

uint32_t packets_count;

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
	g_logger::log("registering irq handlers");
	g_register_irq_handler(1, irq_handler);
	g_register_irq_handler(12, irq_handler);

	// wait for control requests
	g_message_empty(msg);
	g_tid tid = g_get_tid();
	while (true) {
		g_recv_msg(tid, &msg);

		if (msg.type == G_PS2_COMMAND_REGISTER_KEYBOARD) {
			keyboard_receiver_tid = msg.sender;
			keyboard_receiver_transaction = msg.topic;

		} else if (msg.type == G_PS2_COMMAND_REGISTER_MOUSE) {
			mouse_receiver_tid = msg.sender;
			mouse_receiver_transaction = msg.topic;

		} else {
			g_logger::log("ps2driver: received unknown message with type %i",
					msg.type);
		}
	}
}

/**
 *
 */
void irq_handler(uint8_t irq) {

	uint8_t status;
	while (((status = g_utils::inportByte(0x64)) & 1) != 0) {
		uint8_t value = g_utils::inportByte(0x60);

		bool fromKeyboard = ((status & (1 << 5)) == 0);

		if (fromKeyboard) {
			handle_keyboard_data(value);
		} else {
			handle_mouse_data(value);
		}

		++packets_count;
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

			g_message_empty(message);
			message.topic = mouse_receiver_transaction;
			message.parameterA = (((uint16_t) offX) << 16) | ((uint16_t) offY);
			message.parameterB = flags;
			g_send_msg(mouse_receiver_tid, &message);
		}

		mouse_packet_number = 0;
		break;
	}
}

/**
 *
 */
void handle_keyboard_data(uint8_t b) {

	g_message_empty(message);
	message.topic = keyboard_receiver_transaction;
	message.type = 1;
	message.parameterA = b;
	g_send_msg(keyboard_receiver_tid, &message);
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
void write_to_mouse(uint8_t value) {

	// tell the controller that we want to write to the mouse
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x64, 0xD4);

	// send the value to the mouse
	wait_for_buffer(PS2_OUT);
	g_utils::outportByte(0x60, value);
}

