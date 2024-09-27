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
#include <libfont/font_loader.hpp>
#include <libinput/keyboard/keyboard.hpp>
#include <list>
#include <stdio.h>
#include <string.h>

static std::string defaultFont = "consolas";

bool gui_screen_t::initialize()
{
	inputBufferLock = g_atomic_initialize();
	inputBufferEmpty = g_atomic_initialize();
	g_atomic_lock(inputBufferEmpty);

	if (!create_ui()) return false;

	font = g_font_loader::get(defaultFont);

	document = new terminal_document();

	g_create_thread_d((void *) paintEntry, this);
	g_create_thread_d((void *) blinkCursorEntry, this);

	return true;
}

bool gui_screen_t::create_ui()
{
	auto status = g_ui::open();
	if (status != G_UI_OPEN_STATUS_SUCCESSFUL)
	{
		klog("terminal: failed to initialize g_ui with status %i", status);
		return false;
	}

	window = g_window::create();
	window->setTitle("Terminal");
	window->setBackground(ARGB(250, 0, 0, 2));

	canvas = g_canvas::create();
	canvas->setBufferListener(new canvas_buffer_listener_t(this));

	window->setLayout(G_UI_LAYOUT_MANAGER_GRID);
	window->addChild(canvas);

	g_rectangle windowBounds = g_rectangle(80, 80, 700, 500);
	window->setBounds(windowBounds);
	window->setVisible(true);

	canvas->setBoundsListener(new canvas_resize_bounds_listener_t(this));
	canvas->setListener(G_UI_COMPONENT_EVENT_TYPE_KEY, new input_key_listener_t(this));
	window->setListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, new terminal_focus_listener_t(this));

	window->onClose([]()
	{
		g_exit(0);
	});
	return true;
}

void gui_screen_t::blinkCursorEntry(gui_screen_t *screen)
{
	while (true)
	{
		screen->blinkCursor();
		g_sleep(500);
	}
}

void gui_screen_t::paintEntry(gui_screen_t *screen)
{
	screen->paint();
}

void gui_screen_t::blinkCursor()
{
	cursorBlink = !cursorBlink;
	repaint();
}

void canvas_resize_bounds_listener_t::handle_bounds_changed(g_rectangle bounds)
{
	screen->setCanvasBounds(bounds);
	screen->repaint();
}

void input_key_listener_t::handle_key_event(g_key_event &e)
{
	auto info = g_keyboard::fullKeyInfo(e.info);
	if (info.key == "KEY_PAD_2")
	{
		screen->scroll(1);
	} else if (info.key == "KEY_PAD_8")
	{
		screen->scroll(-1);
	} else
	{
		screen->bufferInput(info);
	}
}

g_key_info gui_screen_t::readInput()
{
	g_key_info result;

	for (;;)
	{
		g_atomic_lock(inputBufferLock);
		if (inputBuffer.size() == 0)
		{
			g_atomic_unlock(inputBufferLock);
			g_atomic_lock(inputBufferEmpty);
		} else
		{
			result = inputBuffer.front();
			inputBuffer.pop_front();
			g_atomic_unlock(inputBufferLock);
			break;
		}
	}

	return result;
}

void gui_screen_t::bufferInput(const g_key_info &info)
{
	g_atomic_lock(inputBufferLock);
	inputBuffer.push_back(info);
	lastInputTime = g_millis();
	g_atomic_unlock(inputBufferEmpty);
	g_atomic_unlock(inputBufferLock);
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

char_layout_t *gui_screen_t::getCharLayout(cairo_scaled_font_t *scaledFont, char c)
{
	auto entry = charLayoutCache.find(c);
	if (entry != charLayoutCache.end())
	{
		return (*entry).second;
	}

	// Perform layouting
	char cbuf[2];
	cbuf[0] = c;
	cbuf[1] = 0;

	char_layout_t *layout = new char_layout_t();
	cairo_text_cluster_flags_t cluster_flags;
	cairo_status_t stat = cairo_scaled_font_text_to_glyphs(scaledFont, 0, 0, cbuf, 1, &layout->glyph_buffer,
	                                                       &layout->glyph_count, &layout->cluster_buffer,
	                                                       &layout->cluster_count, &cluster_flags);

	if (stat == CAIRO_STATUS_SUCCESS)
	{
		charLayoutCache[c] = layout;
		return layout;
	}
	return 0;
}

void gui_screen_t::printChar(cairo_t *cr, cairo_scaled_font_t *scaledFont, int x, int y, char c,
                             bool blink_on)
{
	char_layout_t *char_layout = getCharLayout(scaledFont, c);
	cairo_text_extents_t char_extents;
	cairo_scaled_font_glyph_extents(scaledFont, char_layout->glyph_buffer, 1, &char_extents);

	if (!char_layout)
		return;

	cairo_save(cr);
	if (cursorX == x && cursorY == y && blink_on && false /* TODO use for selection cursor */)
	{
		cairo_set_source_rgba(cr, 0, 0, 0, 1);
	} else
	{
		//switch (value.foreground)
		//{
		//	case SC_RED:
		//		cairo_set_source_rgba(cr, 0.8, 0.2, 0.2, 1);
		//		break;
		//	case SC_WHITE:
		//	default:
		cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
		//		break;
		//}
	}
	cairo_translate(cr,
	                x * charWidth + viewPadding,
	                y * charHeight + charHeight - 3 + viewPadding);
	cairo_glyph_path(cr, char_layout->glyph_buffer, char_layout->cluster_buffer[0].num_glyphs);
	cairo_fill(cr);
	cairo_restore(cr);
}

void gui_screen_t::extendRect(g_rectangle &changed, int x, int y)
{
	if (changed.x == -1)
	{
		changed.x = x;
		changed.width = 1;
	} else
	{
		if (x < changed.x)
		{
			int diff = changed.x - x;
			changed.x = x;
			changed.width += diff;
		} else if (x >= changed.x + changed.width)
		{
			changed.width = x - changed.x + 1;
		}
	}

	if (changed.y == -1)
	{
		changed.y = y;
		changed.height = 1;
	} else
	{
		if (y < changed.y)
		{
			int diff = changed.y - y;
			changed.y = y;
			changed.height += diff;
		} else if (y >= changed.y + changed.height)
		{
			changed.height = y - changed.y + 1;
		}
	}
}


void gui_screen_t::paint()
{
	cairo_scaled_font_t *scaledFont = nullptr;
	cairo_font_options_t *fontOptions = nullptr;
	fullRepaint = true;

	viewBufferMax = 300 * 80;
	viewBuffer = new char[viewBufferMax];

	volatile int lastPaintedCursorX = -1;
	volatile int lastPaintedCursorViewY = -1;

	while (true)
	{
		auto cr = getGraphics();
		if (!cr)
		{
			g_sleep(100);
			continue;
		}

		int visibleCols = getColumns();
		int visibleRows = getRows();
		g_rectangle changed = g_rectangle(-1, -1, -1, -1);

		if (fullRepaint)
		{
			fullRepaint = false;

			changed.x = 0;
			changed.y = 0;
			changed.width = visibleCols;
			changed.height = visibleRows;

			// clear everything
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			fontOptions = cairo_font_options_create();
			cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
		}

		// check view-buffer is valid
		if (viewBufferCols != visibleCols || viewBufferRows != visibleRows)
		{
			g_size newLen = visibleRows * visibleCols;
			if (newLen > viewBufferMax)
			{
				g_size newMax = viewBufferMax * 2;
				char *newViewBuffer = new char[newMax];
				char *oldViewBuffer = viewBuffer;
				viewBuffer = newViewBuffer;
				viewBufferMax = newMax;

				delete[] oldViewBuffer;
			}

			memset(viewBuffer, 0, newLen);
			viewBufferRows = visibleRows;
			viewBufferCols = visibleCols;

			changed.x = 0;
			changed.y = 0;
			changed.width = visibleCols;
			changed.height = visibleRows;
		}

		// Calculate what is to render into the view buffer
		int renderStartRow;
		if (documentRows < visibleRows)
		{
			renderStartRow = documentRows;
		} else
		{
			renderStartRow = visibleRows + scrollY;
		}
		for (int y = 0; y < visibleRows; y++)
		{
			int currentRow = renderStartRow - y - 1;
			auto line = document->get_row(currentRow, visibleCols);

			for (int x = 0; x < visibleCols; x++)
			{
				auto c = line.start ? (line.length > x ? line.start[x] : 0) : 0;

				if (viewBuffer[y * visibleCols + x] != c)
				{
					viewBuffer[y * visibleCols + x] = c;
					extendRect(changed, x, y);
				}
			}
		}

		// Include previous cursor position in "changed" rectangle
		int cursorViewY = renderStartRow + cursorY - 1;
		int cursorViewX = (cursorX - 1) % visibleCols + 1;

		if (lastPaintedCursorX != cursorX || lastPaintedCursorViewY != cursorViewY)
		{
			extendRect(changed, lastPaintedCursorX, lastPaintedCursorViewY);
			lastPaintedCursorX = cursorX;
			lastPaintedCursorViewY = cursorViewY;
		}


		// prepare font
		cairo_set_font_face(cr, font->getFace());
		cairo_set_font_size(cr, fontSize);
		scaledFont = cairo_get_scaled_font(cr);
		cairo_set_font_options(cr, fontOptions);

		// clear changed
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr,
		                changed.x * charWidth + viewPadding,
		                changed.y * charHeight + viewPadding,
		                changed.width * charWidth,
		                changed.height * charHeight);
		cairo_fill(cr);
		cairo_restore(cr);

		// Paint on screen
		for (int y = 0; y < visibleRows; y++)
		{
			for (int x = 0; x < visibleCols; x++)
			{
				auto c = viewBuffer[y * visibleCols + x];
				if (c)
				{
					if ((x >= changed.x) && (y >= changed.y)
					    && (x <= changed.x + changed.width)
					    && (y <= changed.y + changed.height))
					{
						printChar(cr, scaledFont, x, y, c, false);
					}
				}
			}
		}

		// paint cursor
		bool blink_on = false;
		if (focused)
		{
			blink_on = (g_millis() - lastInputTime < 300) || cursorBlink;

			if (blink_on)
			{
				cairo_save(cr);
				cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
				cairo_rectangle(cr,
				                cursorViewX * charWidth + viewPadding,
				                cursorViewY * charHeight + viewPadding,
				                cursorWidth,
				                charHeight);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}

		canvas->blit(g_rectangle(0, 0, canvasBounds.width, canvasBounds.height));

		g_atomic_lock(upToDate);
	}
}

cairo_t *gui_screen_t::getGraphics()
{
	auto bufferInfo = canvas->getBuffer();
	if (bufferInfo.buffer == 0)
		return 0;

	bufferSize.width = bufferInfo.width;
	bufferSize.height = bufferInfo.height;

	// get the surface ready and go:
	if (existingSurface == 0 || existingSurfaceBuffer != bufferInfo.buffer)
	{
		if (existingContext != 0)
		{
			cairo_destroy(existingContext);
		}

		existingSurface = cairo_image_surface_create_for_data(bufferInfo.buffer, CAIRO_FORMAT_ARGB32,
		                                                      bufferInfo.width, bufferInfo.height,
		                                                      cairo_format_stride_for_width(
			                                                      CAIRO_FORMAT_ARGB32, bufferInfo.width));
		existingSurfaceBuffer = bufferInfo.buffer;
		existingContext = cairo_create(existingSurface);
	}

	return existingContext;
}

bool charIsUtf8(char c)
{
	if ((c == 0x09 || c == 0x0A || c == 0x0D || (0x20 <= c && c <= 0x7E)))
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
	document->remove(cursorX, cursorY);
	recalculateView();
}

void gui_screen_t::backspace()
{
	setCursor(cursorX - 1, cursorY);
	document->remove(cursorX, cursorY);
	recalculateView();
}

void gui_screen_t::recalculateView()
{
	documentRows = document->get_total_rows(getColumns());
	normalizeScroll();
}

void gui_screen_t::write(char c)
{
	if (!charIsUtf8(c))
		return;

	document->insert(cursorX, cursorY, c);
	recalculateView();

	if (c == '\n')
	{
		setCursor(0, cursorY);
	} else
	{
		setCursor(cursorX + 1, cursorY);
	}
}


void gui_screen_t::setCursor(int x, int y)
{
	cursorX = x;
	cursorY = y;
}

int gui_screen_t::getCursorX()
{
	return cursorX;
}

int gui_screen_t::getCursorY()
{
	return cursorY;
}

void gui_screen_t::flush()
{
	repaint();
}

void gui_screen_t::repaint()
{
	g_atomic_unlock(upToDate);
}

void gui_screen_t::setFocused(bool _focused)
{
	lastInputTime = g_millis();
	focused = _focused;
}

int gui_screen_t::getColumns()
{
	return (canvasBounds.width - 2 * viewPadding) / charWidth;
}

int gui_screen_t::getRows()
{
	return (canvasBounds.height - 2 * viewPadding) / charHeight;
}

void gui_screen_t::setCanvasBounds(g_rectangle &bounds)
{
	canvasBounds = bounds;
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

	if (scrollY > documentRows)
	{
		scrollY = documentRows;
	}

	if (viewRows < documentRows)
	{
		if (scrollY > documentRows - viewRows)
		{
			scrollY = documentRows - viewRows;
		} else if (scrollY < -viewRows + 1)
		{
			scrollY = -viewRows + 1;
		}
	} else
	{
		scrollY = 0;
	}
}

void gui_screen_t::setCursorVisible(bool visible)
{
	// TODO
}
