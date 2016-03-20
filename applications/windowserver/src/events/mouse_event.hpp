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

#ifndef __MOUSE_EVENT__
#define __MOUSE_EVENT__

#include <events/event.hpp>
#include <events/locatable.hpp>

/**
 *
 */
typedef uint8_t mouse_button_t;
#define MOUSE_BUTTON_NONE	0x0
#define MOUSE_BUTTON_1		0x1
#define MOUSE_BUTTON_2		0x2
#define MOUSE_BUTTON_3		0x4

/**
 *
 */
enum mouse_event_type_t {
	MOUSE_EVENT_NONE, MOUSE_EVENT_MOVE, MOUSE_EVENT_PRESS, MOUSE_EVENT_RELEASE, MOUSE_EVENT_DRAG_RELEASE, MOUSE_EVENT_DRAG, MOUSE_EVENT_ENTER, MOUSE_EVENT_LEAVE
};

/**
 *
 */
class mouse_event_t: public event_t, public locatable_t {
public:
	mouse_event_t() :
			type(MOUSE_EVENT_NONE), buttons(MOUSE_BUTTON_NONE), clickCount(1) {
	}

	mouse_event_type_t type;
	mouse_button_t buttons;
	int clickCount;
};

#endif
