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

#include <libvmsvgadriver/vmsvgadriver.hpp>
#include <ghost.h>
#include <cstdio>

bool initialized = false;

int main()
{
	klog("vmsvgadriver");
	if(!g_task_register_id(G_VMSVGA_DRIVER_IDENTIFIER))
	{
		klog("vmsvgadriver: could not register with task identifier '%s'", (char*) G_VMSVGA_DRIVER_IDENTIFIER);
		return -1;
	}

	initialized = svgaInitializeDevice();
	if(!initialized)
	{
		klog("failed to initialize SVGA controller");
	}

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
		{
			continue;
		}

		auto header = (g_message_header*) buf;
		auto request = (g_vmsvga_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_VMSVGA_COMMAND_SET_MODE)
		{
			auto modeSetRequest = (g_vmsvga_set_mode_request*) request;

			g_vmsvga_set_mode_response response;
			if(initialized)
			{
				klog("vmsvgadriver: setting video mode to %ix%i@%i", modeSetRequest->width, modeSetRequest->height,
				     modeSetRequest->bpp);
				svgaSetMode(modeSetRequest->width, modeSetRequest->height, modeSetRequest->bpp);

				void* addressInRequestersSpace = g_share_mem(svgaGetFb(), svgaGetFbSize(), header->sender);

				response.status = G_VMSVGA_SET_MODE_STATUS_SUCCESS;
				response.mode_info.lfb = (uint32_t) addressInRequestersSpace;
				response.mode_info.resX = modeSetRequest->width; // TODO read back from SVGA registers
				response.mode_info.resY = modeSetRequest->height;
				response.mode_info.bpp = (uint8_t) modeSetRequest->bpp;
				response.mode_info.bpsl = (uint16_t) (modeSetRequest->width * 4); // TODO
			}
			else
			{
				response.status = G_VMSVGA_SET_MODE_STATUS_FAILED;
			}
			g_send_message_t(header->sender, &response, sizeof(g_vmsvga_set_mode_response), header->transaction);
		}
		else if(request->command == G_VMSVGA_COMMAND_UPDATE)
		{
			if(initialized)
			{
				auto updateRequest = (g_vmsvga_update_request*) request;
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
