/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifdef _GHOST_

#include "platform/platform.hpp"
#include "generic_video_output.hpp"

g_video_output* platformCreateVideoOutput()
{
#ifdef _GHOST_
	g_set_video_log(false);
	platformLog("waiting for a video device to be present...");

	auto tx = G_MESSAGE_TOPIC_TRANSACTION_START;
	size_t bufLen = 1024;
	uint8_t buf[bufLen];
	while(true)
	{
		auto status = g_receive_topic_message(G_DEVICE_EVENT_TOPIC, buf, bufLen, tx);
		if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto message = (g_message_header*) buf;
			tx = message->transaction;
			auto content = (g_device_event_header*) G_MESSAGE_CONTENT(message);

			if(content->event == G_DEVICE_EVENT_DEVICE_REGISTERED)
			{
				auto deviceEvent = (g_device_event_device_registered*) content;

				if(deviceEvent->type == G_DEVICE_TYPE_VIDEO)
				{
					return new g_generic_video_output(deviceEvent->driver, deviceEvent->id);
				}
			}
		}
	}
#endif
}

#endif