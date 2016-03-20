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
#if SELECTED_TEST == TEST_MESSAGING

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
	g_message_header* buf = (g_message_header*) new uint8_t[buflen];

	while (true) {
		for (int i = 0; i < 1000; i++) {
			auto stat = g_receive_message(buf, buflen);

			if (stat != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
				klog("failed receiving with status %i", stat);
				break;
			}
		}

		g_message_header* message = (g_message_header*) buf;
		klog("content: %i times * %i = %i, '%s'", 1000, message->length,
				1000 * message->length, G_MESSAGE_CONTENT(message));
	}
}

/**
 *
 */
void message_test_sender() {

	g_sleep(1000);

	klog("starting to send messages");
	for (;;) {

		const char* data = "Hello messaging";
		size_t datalen = strlen(data);

		int c = 4000;
		auto start = g_millis();
		for (int i = 0; i < c; i++) {
			g_send_message(message_receiver_tid, (void*) data, datalen);
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
