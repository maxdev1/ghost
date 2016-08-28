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

#include "screen.hpp"

#include <cairo/cairo.h>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/canvas.hpp>

#include <ghostuser/utils/utils.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>
#include <ghostuser/ui/focus_listener.hpp>
#include <ghostuser/ui/key_listener.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <ghostuser/graphics/text/font_loader.hpp>
#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>

/**
 *
 */
class gui_screen_t: public screen_t {
private:
	g_window* window;
	g_canvas* canvas;

	cairo_surface_t* existingSurface = 0;
	g_color_argb* existingSurfaceBuffer = 0;
	g_dimension bufferSize;
	cairo_t* existingContext;

	g_layouted_text* viewModel;
	g_font* font;

	void initialize();
	cairo_t* getGraphics();

	static void blinkCursorThread();

public:
	gui_screen_t();

	static void paint_entry();
	void paint();

	void clean();
	void deactivate();
	void activate();

	void backspace();
	void write(std::string message, screen_color_t color = SC_DEFAULT_COLOR);
	void writeChar(char c, screen_color_t color = SC_DEFAULT_COLOR);
	void updateCursor();

	g_key_info readInput(bool* cancelCondition);

	void workingDirectoryChanged(std::string str);
};
