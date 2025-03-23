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

#include "efifbdriver.hpp"

#include <libdevice/manager.hpp>
#include <ghost.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

g_device_id deviceId;

int main()
{
	g_task_register_name("efifbdriver");

	if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_VIDEO, g_get_tid(), &deviceId))
	{
		klog("failed to register device with device manager");
		return -1;
	}
	klog("registered EFI FB device %i", deviceId);

	efifbDriverReceiveMessages();
	return 0;
}

void efifbDriverReceiveMessages()
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

		g_message_header* header = (g_message_header*) buf;
		g_video_request_header* request = (g_video_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_VIDEO_COMMAND_SET_MODE)
		{
			efifbDriverHandleCommandSetMode((g_video_set_mode_request*) request, header->sender, header->transaction);
		}
		else
		{
			klog("efifbdriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}

void efifbDriverHandleCommandSetMode(g_video_set_mode_request* request, g_tid requestingTaskId,
                                     g_message_transaction requestTransaction)
{
	g_address lfb;
	uint16_t resX;
	uint16_t resY;
	uint16_t bpp;
	uint32_t pitch;
	g_get_efi_framebuffer(&lfb, &resX, &resY, &bpp, &pitch);

	uint64_t lfbSize = pitch * resY;
	auto localMapped = g_map_mmio((void*) lfb, lfbSize);
	// TODO: This is kind of unneccessary, we don't want to map it here
	void* addressInRequestersSpace = g_share_mem((void*) localMapped, lfbSize, requestingTaskId);

	g_video_set_mode_response response{};
	response.status = G_VIDEO_SET_MODE_STATUS_SUCCESS;
	response.mode_info.lfb = (g_address) addressInRequestersSpace;
	response.mode_info.resX = resX;
	response.mode_info.resY = resY;
	response.mode_info.bpp = (uint8_t) bpp;
	response.mode_info.bpsl = (uint16_t) pitch;
	response.mode_info.explicit_update = false;
	g_send_message_t(requestingTaskId, &response, sizeof(g_video_set_mode_response), requestTransaction);

	klog("width %i, pitch %i, /8: %i", resX, pitch, pitch/8);
}
