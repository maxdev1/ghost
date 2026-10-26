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

#include "ps2driver.hpp"

#include <libps2/ps2.hpp>

#include <ghost.h>
#include <stdint.h>
#include <stdio.h>

g_fd keyboardRead = G_FD_NONE;
g_fd keyboardWrite = G_FD_NONE;
g_fd mouseRead = G_FD_NONE;
g_fd mouseWrite = G_FD_NONE;
static bool driverInitialized = false;

g_tid keyboardPartnerTask = G_TID_NONE;
g_tid mousePartnerTask = G_TID_NONE;

int main()
{
	if(!g_task_register_name(G_PS2_DRIVER_NAME))
	{
		klog("ps2driver: could not register with task name '%s'", (char*) G_PS2_DRIVER_NAME);
		return -1;
	}

	ps2DriverInitialize();
	ps2DriverReceiveMessages();
	return 0;
}

void ps2DriverInitialize()
{
	if(g_pipe_b(&keyboardWrite, &keyboardRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for keyboard data");
		return;
	}

	if(g_pipe_b(&mouseWrite, &mouseRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for mouse data");
		g_close(keyboardRead);
		g_close(keyboardWrite);
		keyboardRead = keyboardWrite = G_FD_NONE;
		return;
	}

	driverInitialized = ps2Initialize(ps2MouseCallback, ps2KeyboardCallback) == G_PS2_STATUS_SUCCESS;
	if(!driverInitialized)
	{
		klog("ps2driver: failed to initialize PS/2 input");
		g_close(keyboardRead);
		g_close(keyboardWrite);
		g_close(mouseRead);
		g_close(mouseWrite);
		keyboardRead = keyboardWrite = mouseRead = mouseWrite = G_FD_NONE;
	}
}

// Called only by libps2's delivery task, never by an IRQ receiver. Keep an
// event until the nonblocking pipe accepts it; hardware draining continues.
static bool ps2WriteInput(g_fd fd, const void* packet, size_t length)
{
	size_t offset = 0;
	while(offset < length)
	{
		g_fs_write_status status;
		int32_t written = g_write_s(fd, (const uint8_t*) packet + offset, length - offset, &status);
		if(written > 0)
			offset += written;
		else if(status == G_FS_WRITE_BUSY)
			g_sleep(1);
		else
		{
			klog("ps2driver: input pipe write failed with status %i", status);
			return false;
		}
	}
	return true;
}

void ps2MouseCallback(int16_t x, int16_t y, uint8_t flags, int8_t scroll)
{
	g_ps2_mouse_packet packet;
	packet.x = x;
	packet.y = y;
	packet.flags = flags;
	packet.scroll = scroll;
	if(!ps2WriteInput(mouseWrite, &packet, sizeof(g_ps2_mouse_packet)))
		return;

	const g_tid partner = __atomic_load_n(&mousePartnerTask, __ATOMIC_ACQUIRE);
	if(partner != G_TID_NONE)
		g_yield_t(partner);
}

void ps2KeyboardCallback(uint8_t c)
{
	if(c == 0x3f) // F5
	{
		g_dump();
	}

	if(!ps2WriteInput(keyboardWrite, &c, 1))
		return;

	const g_tid partner = __atomic_load_n(&keyboardPartnerTask, __ATOMIC_ACQUIRE);
	if(partner != G_TID_NONE)
		g_yield_t(partner);
}

void ps2DriverReceiveMessages()
{
	constexpr size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_initialize_request);
	uint8_t buf[buflen];

	for(;;)
	{
		auto status = g_receive_message(buf, buflen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			klog("error receiving message, retrying");
			continue;
		}

		g_message_header* header = (g_message_header*) buf;
		g_ps2_request_header* request = (g_ps2_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_PS2_COMMAND_INITIALIZE)
		{
			ps2HandleCommandInitialize((g_ps2_initialize_request*) request, header->sender, header->transaction);
		}
		else
		{
			klog("vbedriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}

void ps2HandleCommandInitialize(g_ps2_initialize_request* request, g_tid requestingTaskId,
                                g_message_transaction requestTransaction)
{
	g_ps2_initialize_response response{};
	response.status = G_PS2_INITIALIZE_FAILED;
	response.keyboardRead = G_FD_NONE;
	response.mouseRead = G_FD_NONE;

	if(driverInitialized)
	{
		g_pid targetPid = g_get_pid_for_tid(requestingTaskId);
		g_pid sourcePid = g_get_pid();
		response.keyboardRead = g_clone_fd(keyboardRead, sourcePid, targetPid);
		response.mouseRead = g_clone_fd(mouseRead, sourcePid, targetPid);
		if(response.keyboardRead != G_FD_NONE && response.mouseRead != G_FD_NONE)
		{
			__atomic_store_n(&keyboardPartnerTask, request->keyboardPartnerTask, __ATOMIC_RELEASE);
			__atomic_store_n(&mousePartnerTask, request->mousePartnerTask, __ATOMIC_RELEASE);
			response.status = G_PS2_INITIALIZE_SUCCESS;
		}
	}

	g_send_message_t(requestingTaskId, &response, sizeof(g_ps2_initialize_response), requestTransaction);
}
