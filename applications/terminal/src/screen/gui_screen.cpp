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

void canvas_resize_bounds_listener_t::handle_bounds_changed(g_rectangle bounds)
{
	screen->setCanvasBounds(bounds);
	screen->updateRasterSize();
	screen->repaint();
}

void input_key_listener_t::handle_key_event(g_key_event& e)
{
	screen->bufferInput(g_keyboard::fullKeyInfo(e.info));
}

void canvas_buffer_listener_t::handle_buffer_changed()
{
	screen->updateRasterSize();
	screen->repaint();
}

void terminal_focus_listener_t::handle_focus_changed(bool now_focused)
{
	screen->setFocused(now_focused);
	screen->repaint();
}

bool gui_screen_t::initialize()
{
	raster = new raster_t();

	inputBufferLock = g_atomic_initialize();
	inputBufferEmpty = g_atomic_initialize();
	g_atomic_lock(inputBufferEmpty);

	auto status = g_ui::open();
	if(status != G_UI_OPEN_STATUS_SUCCESSFUL)
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
					{ g_exit(0); });

	font = g_font_loader::get(defaultFont);

	g_create_thread_d((void*) paintEntry, this);
	g_create_thread_d((void*) blinkCursorEntry, this);
	return true;
}

void gui_screen_t::blinkCursorEntry(gui_screen_t* screen)
{
	while(true)
	{
		screen->blinkCursor();
		g_sleep(500);
	}
}

void gui_screen_t::blinkCursor()
{
	cursorBlink = !cursorBlink;
	repaint();
}

void gui_screen_t::paintEntry(gui_screen_t* screen)
{
	screen->paint();
}

char_layout_t* gui_screen_t::getCharLayout(cairo_scaled_font_t* scaledFont, char c)
{
	auto entry = charLayoutCache.find(c);
	if(entry != charLayoutCache.end())
	{
		return (*entry).second;
	}

	// Perform layouting
	char cbuf[2];
	cbuf[0] = c;
	cbuf[1] = 0;

	char_layout_t* layout = new char_layout_t();
	cairo_text_cluster_flags_t cluster_flags;
	cairo_status_t stat = cairo_scaled_font_text_to_glyphs(scaledFont, 0, 0, cbuf, 1, &layout->glyph_buffer, &layout->glyph_count, &layout->cluster_buffer,
														   &layout->cluster_count, &cluster_flags);

	if(stat == CAIRO_STATUS_SUCCESS)
	{
		charLayoutCache[c] = layout;
		return layout;
	}
	return 0;
}

void gui_screen_t::printChar(cairo_t* cr, cairo_scaled_font_t* scaledFont, int x, int y, raster_entry_t& value, bool blink_on)
{

	char_layout_t* char_layout = getCharLayout(scaledFont, value.c);
	cairo_text_extents_t char_extents;
	cairo_scaled_font_glyph_extents(scaledFont, char_layout->glyph_buffer, 1, &char_extents);

	if(!char_layout)
		return;

	cairo_save(cr);
	if(cursorX == x && cursorY == y && blink_on && false /* TODO use for selection cursor */)
	{
		cairo_set_source_rgba(cr, 0, 0, 0, 1);
	}
	else
	{
		switch(value.foreground)
		{
		case SC_RED:
			cairo_set_source_rgba(cr, 0.8, 0.2, 0.2, 1);
			break;
		case SC_WHITE:
		default:
			cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
			break;
		}
	}
	cairo_translate(cr,
					x * charWidth + viewPadding,
					y * charHeight + charHeight - 3 + viewPadding);
	cairo_glyph_path(cr, char_layout->glyph_buffer, char_layout->cluster_buffer[0].num_glyphs);
	cairo_fill(cr);
	cairo_restore(cr);
}

void gui_screen_t::paint()
{
	cairo_scaled_font_t* scaledFont = nullptr;
	cairo_font_options_t* fontOptions = nullptr;
	cairo_t* fontContext = nullptr;

	bool firstPaint = true;
	while(true)
	{
		updateRasterSize();

		auto cr = getGraphics();
		if(!cr)
		{
			g_sleep(100);
			continue;
		}

		if(firstPaint)
		{
			firstPaint = false;

			// clear everything
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			fontOptions = cairo_font_options_create();
			cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
		}

		// prepare font
		cairo_set_font_face(cr, font->getFace());
		cairo_set_font_size(cr, fontSize);
		scaledFont = cairo_get_scaled_font(cr);

		cairo_set_font_options(cr, fontOptions);

		raster->lockBuffer();
		g_rectangle changed = raster->popChanges();

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

		// clear outer padding rect
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, canvasBounds.width, viewPadding);
		cairo_rectangle(cr, canvasBounds.width - viewPadding, 0, viewPadding, canvasBounds.height);
		cairo_rectangle(cr, 0, canvasBounds.height - viewPadding, canvasBounds.width, viewPadding);
		cairo_rectangle(cr, 0, 0, viewPadding, canvasBounds.height);
		cairo_fill(cr);
		cairo_restore(cr);

		// clear cursor
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr,
						1 + cursorX * charWidth + viewPadding,
						cursorY * charHeight + viewPadding,
						cursorWidth,
						charHeight);
		cairo_fill(cr);
		cairo_restore(cr);

		// paint cursor
		bool blink_on = false;
		if(focused)
		{
			blink_on = (g_millis() - lastInputTime < 300) || cursorBlink;

			if(blink_on)
			{
				cairo_save(cr);
				cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
				cairo_rectangle(cr,
								1 + cursorX * charWidth + viewPadding,
								cursorY * charHeight + viewPadding,
								cursorWidth,
								charHeight);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}

		for(int y = changed.y; y < changed.y + changed.height; y++)
		{
			for(int x = changed.x; x < changed.x + changed.width; x++)
			{
				raster_entry_t c = raster->getUnlocked(x, y);
				if(c.c)
					printChar(cr, scaledFont, x, y, c, blink_on);
			}
		}

		raster->unlockBuffer();

		canvas->blit(g_rectangle(
			changed.x * charWidth,
			changed.y * charHeight,
			changed.width * charWidth + 2 * viewPadding,
			changed.height * charHeight + 2 * viewPadding));

		g_atomic_lock(upToDate);
	}
}

cairo_t* gui_screen_t::getGraphics()
{
	auto bufferInfo = canvas->getBuffer();
	if(bufferInfo.buffer == 0)
		return 0;

	bufferSize.width = bufferInfo.width;
	bufferSize.height = bufferInfo.height;

	// get the surface ready and go:
	if(existingSurface == 0 || existingSurfaceBuffer != bufferInfo.buffer)
	{
		if(existingContext != 0)
		{
			cairo_destroy(existingContext);
		}

		existingSurface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width, bufferInfo.height,
															  cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
		existingSurfaceBuffer = bufferInfo.buffer;
		existingContext = cairo_create(existingSurface);
	}

	return existingContext;
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
	raster->clean();
	repaint();
}

void gui_screen_t::backspace()
{
	setCursor(cursorX - 1, cursorY);
	write(' ');
	setCursor(cursorX - 1, cursorY);
}

void gui_screen_t::write(char c)
{
	if(!charIsUtf8(c))
		return;

	if(c == '\n')
	{
		setCursor(0, cursorY + 1);
	}
	else
	{
		raster->put(cursorX, cursorY, c, colorForeground, colorBackground);
		setCursor(cursorX + 1, cursorY);
	}
}

g_key_info gui_screen_t::readInput()
{
	g_key_info result;

	for(;;)
	{
		g_atomic_lock(inputBufferLock);
		if(inputBuffer.size() == 0)
		{
			g_atomic_unlock(inputBufferLock);
			g_atomic_lock(inputBufferEmpty);
		}
		else
		{
			result = inputBuffer.front();
			inputBuffer.pop_front();
			g_atomic_unlock(inputBufferLock);
			break;
		}
	}

	return result;
}

void gui_screen_t::setCursor(int x, int y)
{
	raster->dirty(cursorX, cursorY);
	cursorX = x;
	cursorY = y;

	// Break line when cursor is at end of screen
	g_rectangle bounds = canvasBounds;
	bounds.width -= viewPadding * 2;
	bounds.height -= viewPadding * 2;

	if(cursorX >= bounds.width / charWidth)
	{
		cursorX = 0;
		cursorY++;
	}
	if(cursorX < 0)
	{
		cursorX = raster->getWidth() - 1;
		cursorY--;
	}

	// Scroll screen if required
	int yscroll = (cursorY >= bounds.height / charHeight);
	if(yscroll > 0)
	{
		raster->scrollBy(yscroll);
		cursorY -= yscroll;
	}

	raster->dirty(cursorX, cursorY);
}

int gui_screen_t::getCursorX()
{
	return cursorX;
}

int gui_screen_t::getCursorY()
{
	return cursorY;
}

void gui_screen_t::bufferInput(const g_key_info& info)
{
	g_atomic_lock(inputBufferLock);
	inputBuffer.push_back(info);
	lastInputTime = g_millis();
	g_atomic_unlock(inputBufferEmpty);
	g_atomic_unlock(inputBufferLock);
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

bool gui_screen_t::updateRasterSize()
{
	int newWidth = (canvasBounds.width - 2 * viewPadding) / charWidth;
	int newHeight = (canvasBounds.height - 2 * viewPadding) / charHeight;

	if(newWidth < charWidth)
		newWidth = charWidth;

	if(newHeight < charHeight)
		newHeight = charHeight;

	if(!raster->resizeTo(newWidth, newHeight))
		return false;

	// Ensure cursor is in bounds
	if(cursorX > newWidth)
		cursorX = newWidth - 1;

	if(cursorY > newHeight)
		cursorY = newHeight - 1;

	return true;
}

void gui_screen_t::setScrollAreaScreen()
{
	// TODO
}

void gui_screen_t::setScrollArea(int start, int end)
{
	// TODO
}

void gui_screen_t::scroll(int value)
{
	// TODO
}

void gui_screen_t::setCursorVisible(bool visible)
{
	// TODO
}

int gui_screen_t::getWidth()
{
	return raster->getWidth();
}

int gui_screen_t::getHeight()
{
	return raster->getHeight();
}

void gui_screen_t::setCanvasBounds(g_rectangle& bounds)
{
	canvasBounds = bounds;
}