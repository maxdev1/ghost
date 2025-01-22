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

#include "gui_screen.hpp"
#include <ghost.h>
#include <libinput/keyboard/keyboard.hpp>
#include <list>
#include <cstdio>
#include <cstring>

void guiScreenBlinkCursorEntry(gui_screen_t* screen);
void guiScreenPaintEntry(gui_screen_t* screen);

bool gui_screen_t::initialize()
{
	inputBufferLock = g_mutex_initialize();
	inputBufferEmpty = g_mutex_initialize();
	g_mutex_acquire(inputBufferEmpty);

	if(!createUi())
		return false;

	g_create_thread_d((void*) guiScreenPaintEntry, this);
	g_create_thread_d((void*) guiScreenBlinkCursorEntry, this);
	return true;
}

void guiScreenBlinkCursorEntry(gui_screen_t* screen)
{
	while(true)
	{
		screen->blinkCursor();
		g_sleep(500);
	}
}

void guiScreenPaintEntry(gui_screen_t* screen)
{
	screen->paint();
}

bool gui_screen_t::createUi()
{
	auto status = g_ui::open();
	if(status != G_UI_OPEN_STATUS_SUCCESSFUL)
	{
		klog("terminal: failed to initialize g_ui with status %i", status);
		return false;
	}

	window = g_window::create();
	window->setTitle("Terminal");
	window->setBackground(ARGB(250, 0, 0, 2));

	canvas.component = g_canvas::create();
	canvas.component->setBufferListener(new canvas_buffer_listener_t(this));

	window->setLayout(G_UI_LAYOUT_MANAGER_GRID);
	window->addChild(canvas.component);

	g_rectangle windowBounds = g_rectangle(80, 80, 700, 500);
	window->setBounds(windowBounds);
	window->setVisible(true);

	canvas.component->setBoundsListener(new canvas_resize_bounds_listener_t(this));
	canvas.component->setListener(G_UI_COMPONENT_EVENT_TYPE_KEY, new input_key_listener_t(this));
	window->setListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, new terminal_focus_listener_t(this));

	window->onClose([]()
	{
		g_exit(0);
	});
	return true;
}

void gui_screen_t::blinkCursor()
{
	cursor.blink = !cursor.blink;
	repaint();
}

void canvas_resize_bounds_listener_t::handle_bounds_changed(g_rectangle bounds)
{
	screen->setCanvasBounds(bounds);
	screen->repaint();
}

void input_key_listener_t::handle_key_event(g_key_event& e)
{
	auto info = g_keyboard::fullKeyInfo(e.info);
	if(info.key == "KEY_PAD_2")
	{
		screen->scroll(1);
	}
	else if(info.key == "KEY_PAD_8")
	{
		screen->scroll(-1);
	}
	else
	{
		screen->bufferInput(info);
	}
}

g_key_info gui_screen_t::readInput()
{
	g_key_info result;

	for(;;)
	{
		g_mutex_acquire(inputBufferLock);
		if(inputBuffer.size() == 0)
		{
			g_mutex_release(inputBufferLock);
			g_mutex_acquire(inputBufferEmpty);
		}
		else
		{
			result = inputBuffer.front();
			inputBuffer.pop_front();
			g_mutex_release(inputBufferLock);
			break;
		}
	}

	return result;
}

void gui_screen_t::bufferInput(const g_key_info& info)
{
	g_mutex_acquire(inputBufferLock);
	inputBuffer.push_back(info);
	lastInputTime = g_millis();
	g_mutex_release(inputBufferEmpty);
	g_mutex_release(inputBufferLock);
}

void canvas_buffer_listener_t::handle_buffer_changed()
{
	screen->repaint();
}

void terminal_focus_listener_t::handle_focus_changed(bool now_focused)
{
	screen->setFocused(now_focused);
	screen->repaint();
}

void gui_screen_t::paint()
{
	fullRepaint = true;

	viewBuffer.max = 300 * 80;
	viewBuffer.buffer = new char[viewBuffer.max];

	volatile int lastPaintedCursorX = -1;
	volatile int lastPaintedCursorViewY = -1;

	while(true)
	{
		auto cr = getGraphics();
		if(!cr)
		{
			g_sleep(100);
			continue;
		}

		int visibleCols = getColumns();
		int visibleRows = getRows();
		g_rectangle changed = g_rectangle(-1, -1, -1, -1);

		if(fullRepaint)
		{
			fullRepaint = false;

			changed.x = 0;
			changed.y = 0;
			changed.width = visibleCols;
			changed.height = visibleRows;

			// Clear everything
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);
		}

		// check view-buffer is valid
		if(viewBuffer.columns != visibleCols || viewBuffer.rows != visibleRows)
		{
			g_size newLen = visibleRows * visibleCols;
			if(newLen > viewBuffer.max)
			{
				g_size newMax = viewBuffer.max * 2;
				auto newBuffer = new char[newMax];
				auto oldBuffer = viewBuffer.buffer;

				viewBuffer.buffer = newBuffer;
				viewBuffer.max = newMax;

				delete[] oldBuffer;
			}

			memset(viewBuffer.buffer, 0, newLen);
			viewBuffer.rows = visibleRows;
			viewBuffer.columns = visibleCols;

			changed.x = 0;
			changed.y = 0;
			changed.width = visibleCols;
			changed.height = visibleRows;
		}

		// Calculate what is to render into the view buffer
		int renderStartRow;
		if(documentRows < visibleRows)
		{
			renderStartRow = documentRows;
		}
		else
		{
			renderStartRow = visibleRows + scrollY;
		}

		// Prepare view-buffer
		for(int y = 0; y < visibleRows; y++)
		{
			int currentRow = renderStartRow - y - 1;
			auto row = document->acquireRow(currentRow, visibleCols);

			for(int x = 0; x < visibleCols; x++)
			{
				auto c = row.start ? (row.length > x ? row.start[x] : 0) : 0;

				if(viewBuffer.buffer[y * visibleCols + x] != c)
				{
					viewBuffer.buffer[y * visibleCols + x] = c;
					changed.extend(g_point(x, y));
				}
			}

			document->releaseRow();
		}

		// Include previous cursor position in "changed" rectangle
		int cursorViewY = renderStartRow + cursor.y - 1;
		int cursorViewX = (cursor.x - 1) % visibleCols + 1;

		if(lastPaintedCursorX != cursor.x || lastPaintedCursorViewY != cursorViewY)
		{
			changed.extend(g_point(lastPaintedCursorX, lastPaintedCursorViewY));
			lastPaintedCursorX = cursor.x;
			lastPaintedCursorViewY = cursorViewY;
		}


		// Prepare context for char renderer
		chars.renderer->prepareContext(cr, chars.fontSize);

		// Clear changed character positions
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr,
		                changed.x * chars.width + canvas.padding,
		                changed.y * chars.height + canvas.padding,
		                changed.width * chars.width,
		                changed.height * chars.height);
		cairo_fill(cr);
		cairo_restore(cr);

		// Paint on screen
		for(int y = 0; y < visibleRows; y++)
		{
			for(int x = 0; x < visibleCols; x++)
			{
				auto c = viewBuffer.buffer[y * visibleCols + x];
				if(c == 0)
					continue;

				if((x >= changed.x) && (y >= changed.y)
				   && (x <= changed.x + changed.width)
				   && (y <= changed.y + changed.height))
				{
					chars.renderer->printChar(cr,
					                          x * chars.width + canvas.padding,
					                          y * chars.height + chars.height - 3 + canvas.padding,
					                          c);
				}
			}
		}

		// Paint cursor
		if(focused)
		{
			bool cursorVisible = (g_millis() - lastInputTime < 300) || cursor.blink;

			// Clear cursor location
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(cr,
			                cursorViewX * chars.width + canvas.padding,
			                cursorViewY * chars.height + canvas.padding,
			                cursor.width,
			                chars.height);
			cairo_fill(cr);
			cairo_restore(cr);

			// Paint cursor
			if(cursorVisible)
			{
				cairo_save(cr);
				cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
				cairo_rectangle(cr,
				                cursorViewX * chars.width + canvas.padding,
				                cursorViewY * chars.height + canvas.padding,
				                cursor.width,
				                chars.height);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}

		canvas.component->blit(g_rectangle(0, 0, canvas.bounds.width, canvas.bounds.height));

		g_mutex_acquire(upToDate);
	}
}

cairo_t* gui_screen_t::getGraphics()
{
	auto bufferInfo = canvas.component->getBuffer();
	if(bufferInfo.buffer == nullptr)
		return nullptr;

	canvas.bufferSize.width = bufferInfo.width;
	canvas.bufferSize.height = bufferInfo.height;

	if(canvas.surface == nullptr || canvas.surfaceBuffer != bufferInfo.buffer)
	{
		if(canvas.context != nullptr)
			cairo_destroy(canvas.context);

		canvas.surface = cairo_image_surface_create_for_data(
				bufferInfo.buffer, CAIRO_FORMAT_ARGB32,
				bufferInfo.width, bufferInfo.height,
				cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width)
				);
		canvas.surfaceBuffer = bufferInfo.buffer;
		canvas.context = cairo_create(canvas.surface);
	}

	return canvas.context;
}

bool charIsUtf8(char c)
{
	if((c == 0x09 || c == 0x0A || c == 0x0D || (0x20 <= c && c <= 0x7E)))
	{
		return true;
	}
	return false;
}

void gui_screen_t::clean()
{
	document->clear();
	fullRepaint = true;
	recalculateView();
	repaint();
}

void gui_screen_t::remove()
{
	document->remove(cursor.x, cursor.y);
	recalculateView();
}

void gui_screen_t::backspace()
{
	setCursor(cursor.x - 1, cursor.y);
	document->remove(cursor.x, cursor.y);
	recalculateView();
}

void gui_screen_t::recalculateView()
{
	documentRows = document->getRowCount(getColumns());
	normalizeScroll();
}

void gui_screen_t::write(char c)
{
	if(!charIsUtf8(c))
		return;

	document->insert(cursor.x, cursor.y, c);
	recalculateView();

	if(c == '\n')
	{
		setCursor(0, cursor.y);
	}
	else
	{
		setCursor(cursor.x + 1, cursor.y);
	}
}


void gui_screen_t::setCursor(int x, int y)
{
	cursor.x = x;
	cursor.y = y;
}

int gui_screen_t::getCursorX()
{
	return cursor.x;
}

int gui_screen_t::getCursorY()
{
	return cursor.y;
}

void gui_screen_t::flush()
{
	repaint();
}

void gui_screen_t::repaint() const
{
	g_mutex_release(upToDate);
}

void gui_screen_t::setFocused(bool _focused)
{
	lastInputTime = g_millis();
	focused = _focused;
}

int gui_screen_t::getColumns()
{
	return (canvas.bounds.width - 2 * canvas.padding) / chars.width;
}

int gui_screen_t::getRows()
{
	return (canvas.bounds.height - 2 * canvas.padding) / chars.height;
}

void gui_screen_t::setCanvasBounds(g_rectangle& bounds)
{
	canvas.bounds = bounds;
	recalculateView();
	fullRepaint = true;
}

void gui_screen_t::setScrollAreaScreen()
{
	scrollY = 0;
	fullRepaint = true;
	normalizeScroll();
	repaint();
}

void gui_screen_t::setScrollArea(int start, int end)
{
	scrollY = end;
	fullRepaint = true;
	normalizeScroll();
	repaint();
}

void gui_screen_t::scroll(int value)
{
	scrollY += value;
	fullRepaint = true;
	normalizeScroll();
	repaint();
}

void gui_screen_t::normalizeScroll()
{
	int viewRows = getRows();

	if(scrollY > documentRows)
		scrollY = documentRows;

	if(viewRows < documentRows)
	{
		if(scrollY > documentRows - viewRows)
		{
			scrollY = documentRows - viewRows;
		}
		else if(scrollY < -viewRows + 1)
		{
			scrollY = -viewRows + 1;
		}
	}
	else
	{
		scrollY = 0;
	}
}

void gui_screen_t::setCursorVisible(bool visible)
{
	// TODO
}
