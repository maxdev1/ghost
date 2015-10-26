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

#include "tester.hpp"
#if SELECTED_TEST == TEST_OLD_MESSAGING

#include <iostream>
#include <string>
#include <ghost.h>
#include <string.h>
#include <stdio.h>

static g_tid message_receiver_tid;

/**
 *
 */
void message_test_receiver() {
	message_receiver_tid = g_get_tid();

	size_t buflen = 128;
	g_message msg;

	while (true) {
		int c = 1000;
		for (int i = 0; i < c; i++) {
			auto stat = g_recv_msg(message_receiver_tid, &msg);

			if (stat != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
				klog("failed receiving with status %i", stat);
				break;
			}
		}

		klog("content: %i times * %i = %i", c, sizeof(g_message),
				c * sizeof(g_message));
	}
}

/**
 *
 */
void message_test_sender() {

	g_sleep(1000);

	klog("starting to send old messages");
	for (;;) {

		g_message message;

		int c = 4000;
		auto start = g_millis();
		for (int i = 0; i < c; i++) {
			g_send_msg(message_receiver_tid, &message);
		}
		klog("%i took %i ms", c, g_millis() - start);
		g_sleep(1000);
	}
}

/**
 *
 */
void run_test(int argc, char** argv) {

	g_create_thread((void*) message_test_receiver);
	message_test_sender();
}

#endif
