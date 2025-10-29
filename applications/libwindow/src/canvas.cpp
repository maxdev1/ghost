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
#include "libwindow/platform/platform.hpp"

g_canvas* g_canvas::create()
{
	return createCanvasComponent<g_canvas>();
}

void g_canvas::acknowledgeNewBuffer(uintptr_t address, uint16_t width, uint16_t height)
{
	bool changed = false;

	platformAcquireMutex(currentBufferLock);
	if(address != (uintptr_t) currentBuffer.buffer)
	{
		if(currentBuffer.surface)
		{
			cairo_surface_destroy(currentBuffer.surface);
			currentBuffer.surface = nullptr;
		}
		if(currentBuffer.buffer)
		{
			platformUnmapSharedMemory(currentBuffer.buffer);
		}

		currentBuffer.buffer = (uint8_t*) address;
		currentBuffer.width = width;
		currentBuffer.height = height;

		if(currentBuffer.buffer)
		{
			currentBuffer.surface = cairo_image_surface_create_for_data(
					currentBuffer.buffer, CAIRO_FORMAT_ARGB32,
					currentBuffer.width, currentBuffer.height,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, currentBuffer.width)
					);

			auto surfaceStatus = cairo_surface_status(currentBuffer.surface);
			if(surfaceStatus != CAIRO_STATUS_SUCCESS)
			{
				platformLog("failed to create surface for canvas %i: %i", this->id, surfaceStatus);
				currentBuffer.surface = nullptr;
			}
		}

		changed = true;
	}
	platformReleaseMutex(currentBufferLock);

	if(changed && bufferListener)
		bufferListener->handleBufferChanged();
}

cairo_t* g_canvas::acquireGraphics()
{
	platformAcquireMutex(currentBufferLock);
	if(!currentBuffer.surface)
	{
		platformReleaseMutex(currentBufferLock);
		return nullptr;
	}

	if(!currentBuffer.context)
	{
		currentBuffer.context = cairo_create(currentBuffer.surface);
		auto contextStatus = cairo_status(currentBuffer.context);
		if(contextStatus != CAIRO_STATUS_SUCCESS)
		{
			platformLog("failed to create cairo context: %i", contextStatus);
			currentBuffer.context = nullptr;
			platformReleaseMutex(currentBufferLock);
			return nullptr;
		}
	}

	currentBuffer.contextRefCount++;
	return currentBuffer.context;
}

void g_canvas::releaseGraphics()
{
	--currentBuffer.contextRefCount;
	if(currentBuffer.contextRefCount < 0)
	{
		platformLog("error: over-deref of g_canvas %i by %i references", this->id, currentBuffer.contextRefCount);
		currentBuffer.contextRefCount = 0;
		return;
	}

	if(currentBuffer.contextRefCount == 0 && currentBuffer.context)
	{
		cairo_surface_flush(currentBuffer.surface);
		cairo_destroy(currentBuffer.context);
		currentBuffer.context = nullptr;
	}

	platformReleaseMutex(currentBufferLock);
}

void g_canvas::blit(const g_rectangle& rect)
{
	SYS_TX_T tx = platformCreateMessageTransaction();
	g_ui_component_canvas_blit_request request;
	request.header.id = G_UI_PROTOCOL_CANVAS_BLIT;
	request.id = this->id;
	request.area = rect;
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(g_ui_component_canvas_blit_request), tx);
	platformYieldTo(g_ui_delegate_tid);
}
