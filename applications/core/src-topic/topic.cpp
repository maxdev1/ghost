/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "../src-common/arguments.hpp"
#include "../src-common/io.hpp"
#include <ghost/messages.h>

static int topicReceive(const char* topic, bool follow)
{
	uint8_t buffer[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	auto cursor = G_MESSAGE_TOPIC_TRANSACTION_START;
	auto mode = follow ? G_MESSAGE_RECEIVE_MODE_BLOCKING : G_MESSAGE_RECEIVE_MODE_NON_BLOCKING;
	for(;;)
	{
		auto status = g_receive_topic_message_m(topic, buffer, sizeof(buffer), cursor, mode);
		if(status == G_MESSAGE_RECEIVE_STATUS_EMPTY && !follow)
			return 0;
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			fprintf(stderr, "topic: cannot receive from '%s' (status %d)\n", topic, status);
			return 1;
		}

		auto message = reinterpret_cast<g_message_header*>(buffer);
		if(!coreWriteAll(STDOUT_FILENO, G_MESSAGE_CONTENT(buffer), message->length, "topic", "standard output"))
			return 1;

		cursor = message->transaction;
	}
}

int main(int argc, char** argv)
{
	bool follow = false;
	bool options = true;
	const char* topic = nullptr;
	for(int i = 1; i < argc; ++i)
	{
		if(options && strcmp(argv[i], "--") == 0)
			options = false;
		else if(options && strcmp(argv[i], "-f") == 0)
			follow = true;
		else if(options && coreArgumentIsHelp(argv[i]))
		{
			printf("Usage: topic [-f] TOPIC\nPrint stored topic payloads to standard output.\n"
				   "Use -f to keep waiting for new messages. Payloads are concatenated unchanged.\n");
			return 0;
		}
		else if((options && argv[i][0] == '-') || topic || !argv[i][0])
		{
			fprintf(stderr, "Usage: topic [-f] TOPIC\n");
			return 1;
		}
		else
			topic = argv[i];
	}

	if(!topic)
	{
		fprintf(stderr, "Usage: topic [-f] TOPIC\n");
		return 1;
	}

	return topicReceive(topic, follow);
}
