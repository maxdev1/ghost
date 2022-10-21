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

#ifndef __TERMINAL_SCREEN_GUISCREEN__
#define __TERMINAL_SCREEN_GUISCREEN__

#include "raster.hpp"
#include "screen.hpp"

#include <cairo/cairo.h>
#include <list>
#include <map>

#include <libfont/font.hpp>
#include <libwindow/button.hpp>
#include <libwindow/canvas.hpp>
#include <libwindow/label.hpp>
#include <libwindow/listener/action_listener.hpp>
#include <libwindow/listener/focus_listener.hpp>
#include <libwindow/listener/key_listener.hpp>
#include <libwindow/textfield.hpp>
#include <libwindow/ui.hpp>
#include <libwindow/window.hpp>

struct char_layout_t
{
	cairo_glyph_t* glyph_buffer = nullptr;
	int glyph_count;
	cairo_text_cluster_t* cluster_buffer = nullptr;
	int cluster_count;
};

class canvas_resize_bounds_listener_t;
class input_key_listener_t;
class canvas_buffer_listener_t;
class terminal_focus_listener_t;

class gui_screen_t : public screen_t
{
  private:
	g_window* window;
	g_canvas* canvas;

	g_font* font;

	cairo_surface_t* existingSurface = 0;
	uint8_t* existingSurfaceBuffer = 0;
	g_dimension bufferSize;
	cairo_t* existingContext = 0;

	std::list<g_key_info> inputBuffer;
	g_atom inputBufferEmpty;
	g_atom inputBufferLock;
	void bufferInput(const g_key_info& info);

	g_atom upToDate = g_atomic_initialize();
	bool focused = false;
	uint64_t lastInputTime = 0;

	raster_t* raster;
	int viewPadding = 3;
	bool cursorBlink = false;
	int cursorX = 0;
	int cursorY = 0;
	int cursorWidth = 1;

	g_rectangle canvasBounds;
	void setCanvasBounds(g_rectangle& bounds);

	int charWidth = 8;
	int charHeight = 18;
	uint8_t fontSize = 14.5;
	std::map<char, char_layout_t*> charLayoutCache;
	char_layout_t* getCharLayout(cairo_scaled_font_t* scaledFont, char c);
	void printChar(cairo_t* cr, cairo_scaled_font_t* scaledFont, int x, int y, raster_entry_t& value, bool blinkOn);

	cairo_t* getGraphics();

	static void blinkCursorEntry(gui_screen_t* screen);
	void blinkCursor();

	static void paintEntry(gui_screen_t* screen);
	void paint();

	void repaint();
	void setFocused(bool focused);

	bool updateRasterSize();

	friend class canvas_resize_bounds_listener_t;
	friend class input_key_listener_t;
	friend class canvas_buffer_listener_t;
	friend class terminal_focus_listener_t;

  public:
	/**
	 * Initializes the UI components for the screen.
	 *
	 * @return whether initialization was successful
	 */
	virtual bool initialize();

	virtual g_key_info readInput();
	virtual void clean();
	virtual void backspace();
	virtual void write(char c);
	virtual void flush();

	virtual void setCursor(int x, int y);
	virtual int getCursorX();
	virtual int getCursorY();
	virtual void setCursorVisible(bool visible);

	virtual void setScrollAreaScreen();
	virtual void setScrollArea(int start, int end);
	virtual void scroll(int value);

	virtual int getWidth();
	virtual int getHeight();
};

class canvas_resize_bounds_listener_t : public g_bounds_listener
{
  private:
	gui_screen_t* screen;

  public:
	canvas_resize_bounds_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}

	virtual void handle_bounds_changed(g_rectangle bounds);
};

class input_key_listener_t : public g_key_listener
{
  private:
	gui_screen_t* screen;

  public:
	input_key_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}
	virtual void handle_key_event(g_key_event& e);
};

class canvas_buffer_listener_t : public g_canvas_buffer_listener
{
  private:
	gui_screen_t* screen;

  public:
	canvas_buffer_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}
	virtual void handle_buffer_changed();
};

class terminal_focus_listener_t : public g_focus_listener
{
  private:
	gui_screen_t* screen;

  public:
	terminal_focus_listener_t(gui_screen_t* screen) : screen(screen)
	{
	}
	virtual void handle_focus_changed(bool now_focused);
};

#endif
