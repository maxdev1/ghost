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

#ifndef GHOSTLIBRARY_UI_CANVAS
#define GHOSTLIBRARY_UI_CANVAS

#include <ghostuser/ui/component.hpp>
#include <ghostuser/graphics/graphics.hpp>
#include <cstdint>

/**
 *
 */
struct g_canvas_buffer_info {
	g_color_argb* buffer;
	uint16_t width;
	uint16_t height;
};

/**
 *
 */
class g_canvas: public g_component {
protected:
	g_graphics* graphics;
	g_address buffer;
	g_address new_buffer;

	g_canvas(uint32_t id) :
			graphics(0), g_component(id), buffer(0), new_buffer(0) {
	}

	~g_canvas();

public:
	static g_canvas* create();

	void acknowledgeNewBuffer(g_address address);

	void blit(g_rectangle rect);
	g_canvas_buffer_info getBuffer();
};

#endif
