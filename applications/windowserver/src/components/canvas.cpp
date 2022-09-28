/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "components/canvas.hpp"
#include <ghost/memory.h>
#include <string.h>

#define ALIGN_UP(value) (value + value % 50)

canvas_t::canvas_t(g_tid partnerThread) : partnerThread(partnerThread)
{
	partnerProcess = g_get_pid_for_tid(partnerThread);

	currentBuffer.localMapping = nullptr;
	nextBuffer.localMapping = nullptr;

	mustCheckAgain = false;

	asyncInfo = new async_resizer_info_t();
	asyncInfo->canvas = this;
	asyncInfo->alive = true;
	asyncInfo->lock = 0;
	asyncInfo->checkAtom = 1;
	g_create_thread_d((void*) canvas_t::asyncBufferResizer, asyncInfo);
}

/**
 * The asyncBufferResizer runs a method on our canvas, so we must use this lock
 * to avoid destroying the canvas while it is executed.
 */
canvas_t::~canvas_t()
{
	g_atomic_lock(&asyncInfo->lock);
	asyncInfo->alive = false;
	asyncInfo->lock = 0;
}

void canvas_t::asyncBufferResizer(async_resizer_info_t* asyncInfo)
{
	while(true)
	{
		g_atomic_lock_to(&asyncInfo->checkAtom, 10000);

		g_atomic_lock(&asyncInfo->lock);
		if(!asyncInfo->alive)
			break;
		asyncInfo->canvas->checkBuffer();
		asyncInfo->lock = 0;
	}
	delete asyncInfo;
}

/**
 * When the bounds of a canvas are changed, the buffer must be checked.
 */
void canvas_t::handleBoundChange(g_rectangle oldBounds)
{
	asyncInfo->checkAtom = 0;
}

/**
 * Checks whether the current buffer is still sufficient for the required amount of pixels.
 *
 * If the buffer is not sufficient and was acknowledged, a new buffer is allocated and an event is
 * sent to the client so it knows the new buffer must be acknowledged.
 *
 * If the buffer is not sufficient but was not yet acknowledged by the client, we wait until the current
 * one is acknowledged to then create a new buffer later on.
 */
void canvas_t::checkBuffer()
{
	g_rectangle bounds = getBounds();
	if(bounds.width == 0 || bounds.height == 0)
		return;

	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, ALIGN_UP(bounds.width));
	int requiredSize = G_UI_CANVAS_SHARED_MEMORY_HEADER_SIZE + stride * ALIGN_UP(bounds.height);

	uint16_t requiredPages = G_PAGE_ALIGN_UP(requiredSize) / G_PAGE_SIZE;

	// if next buffer not yet acknowledged, ask client to acknowledge it
	if(nextBuffer.localMapping != nullptr && !nextBuffer.acknowledged)
	{
		mustCheckAgain = true;

		// send event again
		requestClientToAcknowledgeNewBuffer();

		// if there is no buffer yet, create one
	}
	else if(currentBuffer.localMapping == nullptr)
	{
		createNewBuffer(requiredPages);

		// if current buffer is acknowledged but too small, create a new one
	}
	else if(currentBuffer.acknowledged)
	{
		g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) currentBuffer.localMapping;
		if(header->paintable_width < bounds.width || header->paintable_height < bounds.height)
		{
			createNewBuffer(requiredPages);
		}
	}
}

void canvas_t::createNewBuffer(uint16_t requiredPages)
{
	g_rectangle bounds = getBounds();

	// TODO this is leaking memory when creating a buffer before the current "nextBuffer" was acknowledged

	// create a new buffer
	uint32_t bufferSize = requiredPages * G_PAGE_SIZE;
	nextBuffer.acknowledged = false;
	nextBuffer.pages = requiredPages;
	nextBuffer.localMapping = (uint8_t*) g_alloc_mem(bufferSize);

	if(nextBuffer.localMapping == 0)
	{
		klog("warning: failed to allocate a buffer of size %i for a canvas", bufferSize);
		return;
	}

	// share buffer with target process
	nextBuffer.remoteMapping = (uint8_t*) g_share_mem(nextBuffer.localMapping, requiredPages * G_PAGE_SIZE, partnerProcess);

	if(nextBuffer.remoteMapping == 0)
	{
		klog("warning: failed to share a buffer for a canvas to proc %i", partnerProcess);
		g_unmap(nextBuffer.localMapping);
		return;
	}

	// initialize the header
	g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) nextBuffer.localMapping;
	header->paintable_width = ALIGN_UP(bounds.width);
	header->paintable_height = ALIGN_UP(bounds.height);
	header->blit_x = 0;
	header->blit_y = 0;
	header->blit_width = 0;
	header->blit_height = 0;
	header->ready = false;

	requestClientToAcknowledgeNewBuffer();
}

void canvas_t::requestClientToAcknowledgeNewBuffer()
{
	event_listener_info_t listenerInfo;
	if(getListener(G_UI_COMPONENT_EVENT_TYPE_CANVAS_WFA, listenerInfo))
	{
		// create a canvas-wait-for-acknowledge-event
		g_ui_component_canvas_wfa_event event;
		event.header.type = G_UI_COMPONENT_EVENT_TYPE_CANVAS_WFA;
		event.header.component_id = listenerInfo.component_id;
		event.newBufferAddress = (g_address) nextBuffer.remoteMapping;
		g_send_message(listenerInfo.target_thread, &event, sizeof(g_ui_component_canvas_wfa_event));
	}
}

void canvas_t::clientHasAcknowledgedCurrentBuffer()
{
	// previous buffer can be deleted
	if(currentBuffer.localMapping != nullptr)
	{
		g_unmap(currentBuffer.localMapping);
		currentBuffer.localMapping = nullptr;
	}

	currentBuffer = nextBuffer;
	currentBuffer.acknowledged = true;
	nextBuffer.localMapping = 0;

	g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) currentBuffer.localMapping;
	header->ready = false;

	// if the window was resized during an un-acknowledged state, we must now create a new buffer
	if(mustCheckAgain)
	{
		mustCheckAgain = false;
		checkBuffer();
	}
}

void canvas_t::paint()
{
	auto bounds = getBounds();
	auto cr = graphics.getContext();

	// there muts be a buffer that is acknowledged
	if(currentBuffer.localMapping != 0 && currentBuffer.acknowledged)
	{
		g_ui_canvas_shared_memory_header* header = (g_ui_canvas_shared_memory_header*) currentBuffer.localMapping;

		if(header->ready)
		{
			header->ready = false;

			// make background empty
			clearSurface();

			// create a cairo surface from the buffer
			uint8_t* bufferContent = (uint8_t*) (currentBuffer.localMapping + G_UI_CANVAS_SHARED_MEMORY_HEADER_SIZE);
			cairo_surface_t* bufferSurface = cairo_image_surface_create_for_data(bufferContent, CAIRO_FORMAT_ARGB32, header->paintable_width,
																				 header->paintable_height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, header->paintable_width));
			cairo_set_source_surface(cr, bufferSurface, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);

			// mark painted area as dirty
			markDirty(g_rectangle(header->blit_x, header->blit_y, header->blit_width, header->blit_height));
		}
	}
}

void canvas_t::blit()
{
	markFor(COMPONENT_REQUIREMENT_PAINT);
}
