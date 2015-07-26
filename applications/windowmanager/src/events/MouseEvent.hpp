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

#ifndef MOUSEEVENT_HPP_
#define MOUSEEVENT_HPP_

#include <events/Event.hpp>
#include <events/Locatable.hpp>

/**
 *
 */
typedef uint8_t MouseButton;
#define MOUSE_BUTTON_NONE	0x0
#define MOUSE_BUTTON_1		0x1
#define MOUSE_BUTTON_2		0x2
#define MOUSE_BUTTON_3		0x4

/**
 *
 */
enum MouseEventType {
	MOUSE_EVENT_NONE, MOUSE_EVENT_MOVE, MOUSE_EVENT_PRESS, MOUSE_EVENT_RELEASE, MOUSE_EVENT_DRAG_RELEASE, MOUSE_EVENT_DRAG, MOUSE_EVENT_ENTER, MOUSE_EVENT_LEAVE
};

/**
 *
 */
class MouseEvent: public Event, public Locatable {
public:
	MouseEvent() :
			type(MOUSE_EVENT_NONE), buttons(MOUSE_BUTTON_NONE), clickCount(1) {
	}

	MouseEventType type;
	MouseButton buttons;
	int clickCount;
};

#endif
