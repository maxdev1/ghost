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

#ifndef __EVENT_PROCESSOR__
#define __EVENT_PROCESSOR__

#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/io/mouse.hpp>

#include <deque>

#define DEFAULT_MULTICLICK_TIMESPAN	250

/**
 * The event queue is used to store any incoming events for
 * later processing.
 */
class event_processor_t {
public:
	std::deque<g_key_info> keyEventQueue;
	std::deque<g_mouse_info> mouseEventQueue;
	uint32_t multiclickTimespan;

	event_processor_t();

	void queueKeyEvent(g_key_info keyInfo);
	void queueMouseEvent(g_mouse_info mouseInfo);

	void process();

	void translateKeyEvent(g_key_info& info);
	void translateMouseEvent(g_mouse_info& info);

};

#endif
