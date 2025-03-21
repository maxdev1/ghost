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

#include "vmsvgadriver.hpp"
#include "svga.hpp"

#include <libdevice/manager.hpp>
#include <ghost.h>
#include <cstdio>
#include <libvideo/videodriver.hpp>

bool initialized = false;
g_device_id deviceId;

int main()
{
	g_task_register_name("vmsvgadriver");
	klog("started");

	initialized = svgaInitializeDevice();
	if(!initialized)
	{
		klog("failed to initialize SVGA controller");
		return -1;
	}

	if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_VIDEO, g_get_tid(), &deviceId))
	{
		klog("failed to register device with device manager");
		return -1;
	}
	klog("registered VMSVGA device %i", deviceId);

	vmsvgaDriverReceiveMessages();
	return 0;
}

void vmsvgaDriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + 1024;
	uint8_t buf[buflen];

	for(;;)
	{
		auto status = g_receive_message(buf, buflen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;

		auto header = (g_message_header*) buf;
		auto request = (g_video_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_VIDEO_COMMAND_SET_MODE)
		{
			auto modeSetRequest = (g_video_set_mode_request*) request;

			g_video_set_mode_response response{};
			if(initialized)
			{
				klog("vmsvgadriver: setting video mode to %ix%i@%i", modeSetRequest->width, modeSetRequest->height,
				     modeSetRequest->bpp);
				svgaSetMode(modeSetRequest->width, modeSetRequest->height, modeSetRequest->bpp);

				void* addressInRequestersSpace = g_share_mem(svgaGetFb(), svgaGetFbSize(), header->sender);

				response.status = G_VIDEO_SET_MODE_STATUS_SUCCESS;
				response.mode_info.lfb = (g_address) addressInRequestersSpace;
				response.mode_info.resX = modeSetRequest->width; // TODO read back from SVGA registers
				response.mode_info.resY = modeSetRequest->height;
				response.mode_info.bpp = modeSetRequest->bpp;
				response.mode_info.bpsl = (uint16_t) (modeSetRequest->width * 4); // TODO
				response.mode_info.explicit_update = true;
			}
			else
			{
				response.status = G_VIDEO_SET_MODE_STATUS_FAILED;
			}
			g_send_message_t(header->sender, &response, sizeof(response), header->transaction);
		}
		else if(request->command == G_VIDEO_COMMAND_UPDATE)
		{
			if(initialized)
			{
				auto updateRequest = (g_video_update_request*) request;
				svgaUpdate(updateRequest->x, updateRequest->y, updateRequest->width, updateRequest->height);
				g_yield();
			}
		}
		else
		{
			klog("vmsvgadriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}
