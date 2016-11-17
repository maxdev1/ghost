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
#include <ghostuser/ui/canvas_buffer_listener.hpp>
#include <ghostuser/graphics/color_argb.hpp>
#include <cstdint>

/**
 *
 */
struct g_canvas_buffer_info {
	uint8_t* buffer;
	uint16_t width;
	uint16_t height;
};

/**
 *
 */
class g_canvas: public g_component {
protected:
	g_address currentBuffer;
	g_address nextBuffer;

	/**
	 * Listener only for user purpose, so a client gets an event once the
	 * buffer was changed.
	 */
	g_canvas_buffer_listener* userListener;

	g_canvas(uint32_t id) :
			g_component(id), currentBuffer(0), nextBuffer(0), userListener(0) {
	}

public:
	static g_canvas* create();

	void acknowledgeNewBuffer(g_address address);

	void blit(g_rectangle rect);
	g_canvas_buffer_info getBuffer();

	void setBufferListener(g_canvas_buffer_listener* l) {
		userListener = l;
	}
};

#endif
