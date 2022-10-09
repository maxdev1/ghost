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

#include "libwindow/canvas.hpp"
#include "libwindow/listener/canvas_wfa_listener.hpp"

g_canvas_buffer_info g_canvas::getBuffer()
{
	g_canvas_buffer_info info;

	g_atomic_lock(currentBufferLock);
	if(nextBuffer)
	{
		if(currentBuffer)
			g_unmap((void*) currentBuffer);
		currentBuffer = nextBuffer;

		// tell server we have acknowledged the changed buffer
		g_message_transaction tx = g_get_message_tx_id();

		g_ui_component_canvas_ack_buffer_request request;
		request.header.id = G_UI_PROTOCOL_CANVAS_ACK_BUFFER_REQUEST;
		request.id = this->id;
		g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_canvas_ack_buffer_request), tx);

		nextBuffer = 0;
	}

	if(currentBuffer == 0)
	{
		info.buffer = 0;
	}
	else
	{
		info.buffer = (uint8_t*) (currentBuffer + G_UI_CANVAS_SHARED_MEMORY_HEADER_SIZE);
		g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) currentBuffer;
		info.width = header->paintable_width;
		info.height = header->paintable_height;
	}

	g_atomic_unlock(currentBufferLock);

	return info;
}

g_canvas::g_canvas(uint32_t id) : g_component(id), currentBuffer(0), nextBuffer(0), userListener(0)
{
	currentBufferLock = g_atomic_initialize();
}

g_canvas* g_canvas::create()
{
	g_canvas* instance = createComponent<g_canvas, G_UI_COMPONENT_TYPE_CANVAS>();

	if(instance)
		instance->setListener(G_UI_COMPONENT_EVENT_TYPE_CANVAS_WFA, new g_canvas_wfa_listener(instance));

	return instance;
}

void g_canvas::acknowledgeNewBuffer(g_address address)
{
	g_atomic_lock(currentBufferLock);
	if(address == currentBuffer)
		return;
	g_atomic_unlock(currentBufferLock);

	nextBuffer = address;

	if(userListener)
		userListener->handle_buffer_changed();
}

void g_canvas::blit(g_rectangle rect)
{
	g_atomic_lock(currentBufferLock);

	if(currentBuffer)
	{
		// write blit parameters
		g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) currentBuffer;
		header->blit_x = rect.x;
		header->blit_y = rect.y;
		header->blit_width = rect.width;
		header->blit_height = rect.height;
		header->ready = true;

		// send blit message
		g_message_transaction tx = g_get_message_tx_id();
		g_ui_component_canvas_blit_request request;
		request.header.id = G_UI_PROTOCOL_CANVAS_BLIT;
		request.id = this->id;
		g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_canvas_blit_request), tx);
	}

	g_atomic_unlock(currentBufferLock);
}
