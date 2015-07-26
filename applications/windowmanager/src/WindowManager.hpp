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

#ifndef WINDOWMANAGER_HPP_
#define WINDOWMANAGER_HPP_

#include <events/Event.hpp>
#include <ghostuser/graphics/Graphics.hpp>
#include <components/Component.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/io/Keyboard.hpp>
#include <ghostuser/io/Mouse.hpp>
#include <ghostuser/graphics/VBE.hpp>
#include <ghostuser/utils/Logger.hpp>
#include <components/Label.hpp>

#include <cstdint>
#include <deque>

class screen_t;
class Cursor;
class TextRenderer;

#define DEFAULT_MULTICLICK_TIMESPAN	250

/**
 *
 */
class WindowManager {
private:
	uint8_t renderAtom;
	screen_t* screen;

	int multiclickTimespan;

	g_vbe_mode_info graphicsInfo;

	std::deque<g_key_info> keyEventQueue;
	std::deque<g_mouse_info> mouseEventQueue;

	void translateKeyEvent(g_key_info& info);
	void translateMouseEvent(g_mouse_info& info);

public:
	void markForRender();
	void run();
	void systemLoop();
	static WindowManager* getInstance();

	void outputDirty(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source);

	Component* dispatchUpwards(Component* component, Event& event);
	bool dispatch(Component* component, Event& event);

	void queueKeyEvent(g_key_info& info);
	void queueMouseEvent(g_mouse_info& info);

	static Label* getFpsLabel();

	void addToScreen(Component* comp);

	void createTestComponents();

};

#endif
