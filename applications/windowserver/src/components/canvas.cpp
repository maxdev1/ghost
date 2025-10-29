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
#include "process_registry.hpp"
#include "windowserver.hpp"

#include <cstring>

#define ALIGN_UP(value) (value + value % 100)

canvas_t::canvas_t(SYS_TID_T partnerThread)
{
	partnerProcess = platformGetPidForTid(partnerThread);
	asyncInfo = new async_resizer_info_t();
	asyncInfo->canvas = this;
	asyncInfo->alive = true;
	asyncInfo->lock = platformInitializeMutex(false);
	asyncInfo->checkAtom = platformInitializeMutex(false);
	platformCreateThreadWithData((void*) canvas_t::asyncBufferResizer, asyncInfo);
}

/**
 * The asyncBufferResizer runs a method on our canvas, so we must use this lock
 * to avoid destroying the canvas while it is executed.
 */
canvas_t::~canvas_t()
{
	platformAcquireMutex(asyncInfo->lock);
	asyncInfo->alive = false;
	platformReleaseMutex(asyncInfo->lock);
}

void canvas_t::asyncBufferResizer(async_resizer_info_t* asyncInfo)
{
	while(true)
	{
		platformAcquireMutexTimeout(asyncInfo->checkAtom, 10000);

		platformAcquireMutex(asyncInfo->lock);
		if(!asyncInfo->alive)
			break;
		asyncInfo->canvas->checkBuffer();
		platformReleaseMutex(asyncInfo->lock);
	}
	delete asyncInfo;
}

/**
 * When the bounds of a canvas are changed, the buffer must be checked.
 */
void canvas_t::handleBoundChanged(const g_rectangle& oldBounds) { platformReleaseMutex(asyncInfo->checkAtom); }

/**
 * Checks whether the current buffer is still sufficient for the required amount of pixels.
 */
void canvas_t::checkBuffer()
{
	platformAcquireMutex(lock);
	g_rectangle bounds = getBounds();
	if(bounds.width == 0 || bounds.height == 0)
	{
		platformReleaseMutex(lock);
		return;
	}

	bounds.width = ALIGN_UP(bounds.width);
	bounds.height = ALIGN_UP(bounds.height);

	createNewBuffer(bounds, bounds.width, bounds.height);
	platformReleaseMutex(lock);
}

void canvas_t::createNewBuffer(g_rectangle& bounds, int width, int height)
{
	// Check if buffer still large enough
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	int imageSize = stride * height;
	uint16_t pages = SYS_PAGE_ALIGN_UP(imageSize) / SYS_PAGE_SIZE;
	uint32_t bufferSize = pages * SYS_PAGE_SIZE;

	platformAcquireMutex(bufferLock);
	if(pages <= buffer.pages)
	{
		platformReleaseMutex(bufferLock);
		return;
	}

	// Destroy old resources
	if(buffer.surface)
	{
		cairo_surface_destroy(buffer.surface);
		buffer.surface = nullptr;
	}
	if(buffer.localMapping)
	{
		platformUnmapSharedMemory(buffer.localMapping);
		buffer.localMapping = nullptr;
	}

	// create a new buffer
	buffer.pages = pages;
	buffer.localMapping = (uint8_t*) platformAllocateMemory(bufferSize);
	buffer.paintableWidth = bounds.width;
	buffer.paintableHeight = bounds.height;

	if(buffer.localMapping == nullptr)
	{
		platformLog("warning: failed to allocate a buffer of size %i for a canvas", bufferSize);
	}
	else
	{
		memset(buffer.localMapping, 0, bufferSize);
		buffer.remoteMapping = (uint8_t*) platformShareMemory(buffer.localMapping, pages * SYS_PAGE_SIZE, partnerProcess);
		if(buffer.remoteMapping == nullptr)
		{
			platformLog("warning: failed to share a buffer for a canvas to proc %i", partnerProcess);
		}
		else
		{
			buffer.surface = cairo_image_surface_create_for_data(
					buffer.localMapping, CAIRO_FORMAT_ARGB32, buffer.paintableWidth, buffer.paintableHeight,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, buffer.paintableWidth));

			auto status = cairo_surface_status(buffer.surface);
			if(status != CAIRO_STATUS_SUCCESS)
			{
				platformLog("failed to create surface: %i", status);
				buffer.surface = nullptr;
			}
		}
	}

	notifyClientAboutNewBuffer();
	platformReleaseMutex(bufferLock);
}

void canvas_t::notifyClientAboutNewBuffer()
{
	g_ui_component_canvas_wfa_event event;
	event.header.type = G_UI_COMPONENT_EVENT_TYPE_CANVAS_NEW_BUFFER;
	event.header.component_id = this->id;
	event.newBufferAddress = (uintptr_t) buffer.remoteMapping;
	event.width = buffer.paintableWidth;
	event.height = buffer.paintableHeight;

	SYS_TID_T eventDispatcher = process_registry_t::get(partnerProcess);
	if(eventDispatcher == SYS_TID_NONE)
	{
		platformLog("failed to send buffer notification to event dispatcher of process %i since it is not registered",
		     partnerProcess);
	}
	else
	{
		platformSendMessage(eventDispatcher, &event, sizeof(g_ui_component_canvas_wfa_event),SYS_TX_NONE);
	}
}

void canvas_t::blit(graphics_t* out, const g_rectangle& clip, const g_point& positionOnParent)
{
	auto cr = out->acquireContext();
	if(cr)
	{
		platformAcquireMutex(bufferLock);
		if(bufferReady && buffer.surface)
		{
			cairo_surface_mark_dirty(buffer.surface);
			cairo_save(cr);
			cairo_set_source_surface(cr, buffer.surface, positionOnParent.x, positionOnParent.y);
			cairo_paint(cr);
			cairo_restore(cr);
		}
		platformReleaseMutex(bufferLock);

		if(windowserver_t::isDebug())
		{
			cairo_save(cr);
			cairo_set_line_width(cr, 2);
			cairo_rectangle(cr, clip.x, clip.y, clip.width, clip.height);
			cairo_set_source_rgba(cr, 1, 0, 0, 1);
			cairo_stroke(cr);
			cairo_restore(cr);
		}

		out->releaseContext();
	}

	this->blitChildren(out, clip, positionOnParent);
}


void canvas_t::requestBlit(g_rectangle& area)
{
	platformAcquireMutex(bufferLock);
	bufferReady = true;
	platformReleaseMutex(bufferLock);

	markDirty(area);
	windowserver_t::instance()->requestUpdateLater();
}
