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

/**
 * Used to cache information about the layout of a specific character.
 */
struct char_layout_t
{
	cairo_glyph_t* glyph_buffer = nullptr;
	int glyph_count;
	cairo_text_cluster_t* cluster_buffer = nullptr;
	int cluster_count;
};

/**
 *
 */
class gui_screen_t : public screen_t
{
  private:
	/**
	 * UI related properties
	 */
	g_window* window;
	g_canvas* canvas;

	g_font* font;

	cairo_surface_t* existingSurface = 0;
	uint8_t* existingSurfaceBuffer = 0;
	g_dimension bufferSize;
	cairo_t* existingContext = 0;

	g_atom paint_uptodate = g_atomic_initialize();
	bool cursorBlink = false;

	std::list<g_key_info> input_buffer;
	g_atom input_buffer_empty;
	g_atom input_buffer_lock;

	bool focused = false;
	uint64_t last_input_time = 0;

	/**
	 * Screen buffer
	 */
	raster_t* raster;
	int cursor_x = 0;
	int cursor_y = 0;

	int char_width = 8;
	int char_height = 18;
	uint8_t font_size = 14;
	int padding = 3;

	std::map<char, char_layout_t*> char_layout_cache;

	/**
	 * Prepares the canvas buffer for painting.
	 *
	 * @return the cairo instance to paint with
	 */
	cairo_t* getGraphics();

	/**
	 * Thread that is responsible for making the cursor
	 * blink in specific intervals.
	 *
	 * @param screen
	 * 		instance of the screen
	 */
	static void blink_cursor_entry(gui_screen_t* screen);
	void blink_cursor();

	/**
	 *
	 */
	static void paint_entry(gui_screen_t* screen);
	void paint();

	/**
	 *
	 */
	char_layout_t* get_char_layout(cairo_scaled_font_t* scaled_face, char c);

	void printGlyph(cairo_t* cr, cairo_scaled_font_t* scaled_face, int x, int y, uint8_t c, bool blink_on);

  public:
	/**
	 * Initializes the UI components for the screen.
	 *
	 * @return whether initialization was successful
	 */
	bool initialize();

	g_key_info readInput();
	void clean();
	void writeChar(char c);
	void moveCursor(int x, int y);
	int getCursorX();
	int getCursorY();
	void backspace();

	void buffer_input(const g_key_info& info);
	void repaint();
	void set_focused(bool focused);

	void update_visible_buffer_size();
	virtual int getWidth();
	virtual int getHeight();

	void setScrollAreaScreen();
	void setScrollArea(int start, int end);
	void scroll(int value);

	void setCursorVisible(bool visible);
};
