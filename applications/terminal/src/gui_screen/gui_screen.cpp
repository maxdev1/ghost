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

#include "gui_screen/gui_screen.hpp"
#include <ghost.h>
#include <libfont/font_loader.hpp>
#include <libinput/keyboard/keyboard.hpp>
#include <list>
#include <string.h>

class canvas_resize_bounds_listener_t : public g_bounds_listener
{
  public:
	gui_screen_t* screen;

	canvas_resize_bounds_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}

	virtual void handle_bounds_changed(g_rectangle bounds)
	{
		screen->update_visible_buffer_size();
	}
};

class input_key_listener_t : public g_key_listener
{
  private:
	gui_screen_t* screen;

  public:
	/**
	 *
	 */
	input_key_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}

	/**
	 *
	 */
	virtual void handle_key_event(g_key_event& e)
	{
		screen->buffer_input(g_keyboard::fullKeyInfo(e.info));
	}
};

class canvas_buffer_listener_t : public g_canvas_buffer_listener
{
  private:
	gui_screen_t* screen;

  public:
	/**
	 *
	 */
	canvas_buffer_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}

	/**
	 *
	 */
	virtual void handle_buffer_changed()
	{
		screen->update_visible_buffer_size();
		screen->repaint();
	}
};

class terminal_focus_listener_t : public g_focus_listener
{
  private:
	gui_screen_t* screen;

  public:
	/**
	 *
	 */
	terminal_focus_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}

	/**
	 *
	 */
	virtual void handle_focus_changed(bool now_focused)
	{
		screen->set_focused(now_focused);
		screen->repaint();
	}
};

bool gui_screen_t::initialize()
{
	raster = new raster_t();

	input_buffer_lock = g_atomic_initialize();
	input_buffer_empty = g_atomic_initialize();
	g_atomic_lock(input_buffer_empty);

	auto status = g_ui::open();
	if(status != G_UI_OPEN_STATUS_SUCCESSFUL)
	{
		klog("Terminal: Failed to initialize g_ui with status %i", status);
		return false;
	}

	window = g_window::create();
	window->setTitle("Terminal");
	window->setBackground(ARGB(253, 0, 0, 2));

	canvas = g_canvas::create();
	canvas->setBufferListener(new canvas_buffer_listener_t(this));

	window->setLayout(G_UI_LAYOUT_MANAGER_GRID);
	window->addChild(canvas);

	g_rectangle windowBounds = g_rectangle(80, 80, 500, 300);
	window->setBounds(windowBounds);
	window->setVisible(true);

	canvas->setBoundsListener(new canvas_resize_bounds_listener_t(this));
	canvas->setListener(G_UI_COMPONENT_EVENT_TYPE_KEY, new input_key_listener_t(this));
	window->setListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, new terminal_focus_listener_t(this));

	font = g_font_loader::get("consolas");

	g_create_thread_d((void*) paint_entry, this);
	g_create_thread_d((void*) blink_cursor_entry, this);
	return true;
}

void gui_screen_t::blink_cursor_entry(gui_screen_t* screen)
{
	while(true)
	{
		screen->blink_cursor();
		g_sleep(500);
	}
}

void gui_screen_t::blink_cursor()
{
	cursorBlink = !cursorBlink;
	repaint();
}

void gui_screen_t::paint_entry(gui_screen_t* screen)
{
	screen->paint();
}

char_layout_t* gui_screen_t::get_char_layout(cairo_scaled_font_t* scaled_face, char c)
{
	// Take char from layout cache
	auto entry = char_layout_cache.find(c);
	if(entry != char_layout_cache.end())
	{
		return (*entry).second;
	}

	// Perform layouting
	char cbuf[2];
	cbuf[0] = c;
	cbuf[1] = 0;

	char_layout_t* layout = new char_layout_t();
	cairo_text_cluster_flags_t cluster_flags;
	cairo_status_t stat = cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, cbuf, 1, &layout->glyph_buffer, &layout->glyph_count, &layout->cluster_buffer,
														   &layout->cluster_count, &cluster_flags);

	if(stat == CAIRO_STATUS_SUCCESS)
	{
		char_layout_cache[c] = layout;
		return layout;
	}
	return 0;
}

void gui_screen_t::printGlyph(cairo_t* cr, cairo_scaled_font_t* scaled_face, int x, int y, uint8_t c, bool blink_on)
{

	char_layout_t* char_layout = get_char_layout(scaled_face, c);
	cairo_text_extents_t char_extents;
	cairo_scaled_font_glyph_extents(scaled_face, char_layout->glyph_buffer, 1, &char_extents);

	if(!char_layout)
		return;

	cairo_save(cr);
	if(cursor_x == x && cursor_y == y && blink_on)
	{
		cairo_set_source_rgba(cr, 0, 0, 0, 1);
	}
	else
	{
		cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
	}
	cairo_translate(cr,
					x * char_width + padding,
					y * char_height + char_height - 3 + padding);
	cairo_glyph_path(cr, char_layout->glyph_buffer, char_layout->cluster_buffer[0].num_glyphs);
	cairo_fill(cr);
	cairo_restore(cr);
}

void gui_screen_t::paint()
{
	bool firstPaint = true;
	while(true)
	{
		auto windowBounds = window->getBounds();
		update_visible_buffer_size();

		auto cr = getGraphics();
		if(!cr)
		{
			g_sleep(100);
			continue;
		}

		if(firstPaint)
		{
			// clear everything
			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);
			firstPaint = false;
		}

		// prepare font
		cairo_set_font_face(cr, font->getFace());
		cairo_set_font_size(cr, font_size);
		auto scaled_face = cairo_get_scaled_font(cr);

		raster->lockBuffer();
		g_rectangle changed = raster->popChanges();

		// clear changed
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr,
						changed.x * char_width + padding,
						changed.y * char_height + padding,
						changed.width * char_width,
						changed.height * char_height);
		cairo_fill(cr);
		cairo_restore(cr);

		// clear outer padding rect
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, windowBounds.width, padding);
		cairo_rectangle(cr, windowBounds.width - padding, 0, padding, windowBounds.height);
		cairo_rectangle(cr, 0, windowBounds.height - padding, windowBounds.width, padding);
		cairo_rectangle(cr, 0, 0, padding, windowBounds.height);
		cairo_fill(cr);
		cairo_restore(cr);

		// clear cursor
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr,
						cursor_x * char_width + padding,
						cursor_y * char_height + padding,
						char_width,
						char_height);
		cairo_fill(cr);
		cairo_restore(cr);

		// paint cursor
		bool blink_on = false;
		if(focused)
		{
			blink_on = (g_millis() - last_input_time < 300) || cursorBlink;

			if(blink_on)
			{
				cairo_save(cr);
				cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
				cairo_rectangle(cr,
								cursor_x * char_width + padding,
								cursor_y * char_height + padding,
								char_width,
								char_height);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}

		for(int y = changed.y; y < changed.y + changed.height; y++)
		{
			for(int x = changed.x; x < changed.x + changed.width; x++)
			{
				uint8_t c = raster->getUnlocked(x, y);
				if(c)
					printGlyph(cr, scaled_face, x, y, c, blink_on);
			}
		}
		uint8_t underCursorC = raster->getUnlocked(cursor_x, cursor_y);
		if(underCursorC)
			printGlyph(cr, scaled_face, cursor_x, cursor_y, underCursorC, blink_on);

		raster->unlockBuffer();

		canvas->blit(g_rectangle(
			changed.x * char_width,
			changed.y * char_height,
			changed.width * char_width + 2 * padding,
			changed.height * char_height + 2 * padding));

		g_atomic_lock(paint_uptodate);
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
	moveCursor(cursor_x - 1, cursor_y);
	writeChar(' ');
	moveCursor(cursor_x - 1, cursor_y);
	repaint();
}

void gui_screen_t::writeChar(char c)
{
	if(!charIsUtf8(c))
		return;

	if(c == '\n')
	{
		moveCursor(0, cursor_y + 1);
	}
	else
	{
		raster->put(cursor_x, cursor_y, c);
		moveCursor(cursor_x + 1, cursor_y);
	}
	g_atomic_unlock(paint_uptodate);
}

g_key_info gui_screen_t::readInput()
{
	g_key_info result;

	for(;;)
	{
		g_atomic_lock(input_buffer_lock);
		if(input_buffer.size() == 0)
		{
			g_atomic_unlock(input_buffer_lock);
			g_atomic_lock(input_buffer_empty);
		}
		else
		{
			result = input_buffer.front();
			input_buffer.pop_front();
			g_atomic_unlock(input_buffer_lock);
			break;
		}
	}

	return result;
}

void gui_screen_t::moveCursor(int x, int y)
{
	raster->dirty(cursor_x, cursor_y);
	cursor_x = x;
	cursor_y = y;

	// Break line when cursor is at end of screen
	g_rectangle canvas_bounds = canvas->getBounds();
	canvas_bounds.width -= padding * 2;
	canvas_bounds.height -= padding * 2;

	if(cursor_x >= canvas_bounds.width / char_width)
	{
		cursor_x = 0;
		cursor_y++;
	}
	if(cursor_x < 0)
	{
		cursor_x = raster->getWidth() - 1;
		cursor_y--;
	}

	// Scroll screen if required
	int yscroll = (cursor_y >= canvas_bounds.height / char_height);
	if(yscroll > 0)
	{
		raster->scrollBy(yscroll);
		cursor_y -= yscroll;
	}

	raster->dirty(cursor_x, cursor_y);
	repaint();
}

int gui_screen_t::getCursorX()
{
	return cursor_x;
}

int gui_screen_t::getCursorY()
{
	return cursor_y;
}

void gui_screen_t::buffer_input(const g_key_info& info)
{
	g_atomic_lock(input_buffer_lock);
	input_buffer.push_back(info);
	last_input_time = g_millis();
	g_atomic_unlock(input_buffer_empty);
	g_atomic_unlock(input_buffer_lock);
}

void gui_screen_t::repaint()
{
	g_atomic_unlock(paint_uptodate);
}

void gui_screen_t::set_focused(bool _focused)
{
	last_input_time = g_millis();
	focused = _focused;
}

void gui_screen_t::update_visible_buffer_size()
{
	g_rectangle canvas_bounds = canvas->getBounds();
	int newWidth = (canvas_bounds.width - 2 * padding) / char_width;
	int newHeight = (canvas_bounds.height - 2 * padding) / char_height;

	if(newWidth < char_width)
		newWidth = char_width;

	if(newHeight < char_height)
		newHeight = char_height;

	if(!raster->resizeTo(newWidth, newHeight))
		return;

	// Ensure cursor is in bounds
	if(cursor_x > newWidth)
		cursor_x = newWidth - 1;

	if(cursor_y > newHeight)
		cursor_y = newHeight - 1;

	repaint();
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
