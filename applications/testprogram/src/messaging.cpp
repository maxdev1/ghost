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

#include <iostream>
#include <string>
#include <ghost.h>
#include <string.h>
#include <stdio.h>

static g_tid messageReceiverThreadId = 0;
static bool receiverFailed = false;

/**
 *
 */
void messageReceiverThread()
{
	messageReceiverThreadId = g_get_tid();

	size_t buflen = 128;
	g_message_header* buf = (g_message_header*) new uint8_t[buflen];

	char* exp = new char[128];
	for(int j = 0; j < 5; j++)
	{
		for(int i = 0; i < 1000; i++)
		{
			sprintf(exp, "Hello messaging %i %i", j, i);

			auto stat = g_receive_message(buf, buflen);
			if(stat != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			{
				receiverFailed = true;
				return;
			}
			if(strcmp((char*) G_MESSAGE_CONTENT(buf), exp) != 0)
			{
				receiverFailed = true;
				return;
			}
		}
	}
}

test_result_t messageSenderThread()
{
	g_sleep(200);
	ASSERT(messageReceiverThreadId != 0);

	char* buf = new char[128];
	for(int j = 0; j < 5; j++)
	{
		for(int i = 0; i < 1000; i++)
		{
			sprintf(buf, "Hello messaging %i %i", j, i);
			auto stat = g_send_message(messageReceiverThreadId, (void*) buf, strlen(buf) + 1);
			ASSERT(stat == G_MESSAGE_SEND_STATUS_SUCCESSFUL);
		}
		g_sleep(200);

		ASSERT(receiverFailed == false);
	}
	TEST_SUCCESSFUL;
}

test_result_t runMessageTest()
{
	g_tid rectid = g_create_thread((void*) messageReceiverThread);
	messageSenderThread();
	g_join(rectid);
	TEST_SUCCESSFUL;
}
