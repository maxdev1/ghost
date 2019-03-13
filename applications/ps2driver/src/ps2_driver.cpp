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
#include <ghostuser/tasking/lock.hpp>
#include <ghostuser/utils/utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>

/**
 *
 */
uint8_t mouse_packet_number = 0;
uint8_t mouse_packet_buffer[3];

/**
 * Receiver task ids and transaction ids for dispatching incoming packets.
 */
uint32_t mouse_receiver_tid = -1;
uint32_t mouse_receiver_transaction;

uint32_t keyboard_receiver_tid;
uint32_t keyboard_receiver_transaction;

/**
 *
 */
uint64_t packets_count = 0;

g_ps2_shared_area* shared_area;

g_fd keyboard_pipe_w;
g_fd keyboard_pipe_r;

/**
 *
 */
int main() {

	// register
	if (!g_task_register_id(G_PS2_DRIVER_IDENTIFIER)) {
		klog("failed to register as '%s'", (char*) G_PS2_DRIVER_IDENTIFIER);
		return 1;
	}

	// set up shared memory
	shared_area = (g_ps2_shared_area*) g_alloc_mem(sizeof(g_ps2_shared_area));
	if (shared_area == 0) {
		klog("failed to allocate transfer memory area");
		return 1;
	}

	// set up keyboard pipe
	g_fs_pipe_status stat = g_pipe_b(&keyboard_pipe_w, &keyboard_pipe_r, false);
	if (stat != G_FS_PIPE_SUCCESSFUL) {
		klog("failed to create keyboard pipe (status: %i)", stat);
		return 1;
	}

	// initialize memory area
	shared_area->mouse.lock = false;
	shared_area->mouse.buffer_empty_lock = true;
	shared_area->keyboard.buffer_empty_lock = true;
	shared_area->keyboard.buffer_amount = 0;
	shared_area->keyboard.buffer_amount_lock = false;

	// initialize mouse
	initialize_mouse();

	// start receiver threads
	register_operation_mode();

	// wait for control requests
	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_register_request);
	uint8_t* buf = new uint8_t[buflen];
	g_tid tid = g_get_tid();
	while (true) {
		g_message_receive_status stat = g_receive_message(buf, buflen);

		if (stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			g_message_header* mes = (g_message_header*) buf;
			g_ps2_register_request* req =
					(g_ps2_register_request*) G_MESSAGE_CONTENT(buf);

			// share area with requester
			g_pid requester_pid = g_get_pid_for_tid(mes->sender);
			g_ps2_shared_area* shared_in_target =
					(g_ps2_shared_area*) g_share_mem((void*) shared_area,
							sizeof(g_ps2_shared_area), requester_pid);

			// share keyboard pipe with requester
			g_fd keyboard_pipe_target_read_end = g_clone_fd(keyboard_pipe_r,
					g_get_pid(), requester_pid);

			// send response
			g_ps2_register_response response;
			response.area = shared_in_target;
			response.keyboard_pipe = keyboard_pipe_target_read_end;
			g_send_message_t(mes->sender, &response,
					sizeof(g_ps2_register_response), mes->transaction);
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

			// wait for handling and set
			g_atomic_lock(&shared_area->mouse.lock);

			// write data
			shared_area->mouse.move_x += offX;
			shared_area->mouse.move_y += offY;
			shared_area->mouse.flags = flags;
			shared_area->mouse.buffer_empty_lock = false;

			// show waiter that data is queue
			shared_area->mouse.lock = false;
		}

		mouse_packet_number = 0;
		break;
	}
}

/**
 *
 */
void handle_keyboard_data(uint8_t b) {

	// write byte to keyboard pipe
	g_atomic_lock(&shared_area->keyboard.buffer_amount_lock);

	shared_area->keyboard.buffer_empty_lock = false;
	g_write(keyboard_pipe_w, &b, 1);

	// increase buffer ocunter
	shared_area->keyboard.buffer_amount++;
	shared_area->keyboard.buffer_amount_lock = false;
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

