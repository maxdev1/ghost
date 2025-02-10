/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "events/event_processor.hpp"
#include "component_registry.hpp"
#include "components/cursor.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "events/event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"
#include "windowserver.hpp"

event_processor_t::event_processor_t()
{
	multiclickTimespan = DEFAULT_MULTICLICK_TIMESPAN;
}

void event_processor_t::bufferKeyEvent(g_key_info keyInfo)
{
	g_mutex_acquire(key_info_buffer_lock);
	key_info_buffer.push_back(keyInfo);
	g_mutex_release(key_info_buffer_lock);
}

void event_processor_t::process()
{
	processMouseState();
	processKeyState();
}

void event_processor_t::processKeyState()
{
	g_mutex_acquire(key_info_buffer_lock);
	while(key_info_buffer.size() > 0)
	{
		translateKeyEvent(key_info_buffer.back());
		key_info_buffer.pop_back();
	}
	g_mutex_release(key_info_buffer_lock);
}

void event_processor_t::translateKeyEvent(g_key_info& info)
{
	if(cursor_t::focusedComponent != -1)
	{
		auto focusedComponent = component_registry_t::get(cursor_t::focusedComponent);

		if(focusedComponent)
		{
			key_event_t k;
			k.info = info;
			windowserver_t::instance()->dispatch(focusedComponent, k);
		}
	}
}

void event_processor_t::processMouseState()
{
	g_point previousPosition = cursor_t::position;
	g_mouse_button previousPressedButtons = cursor_t::pressedButtons;

	windowserver_t* instance = windowserver_t::instance();
	screen_t* screen = instance->screen;

	if(cursor_t::position != cursor_t::nextPosition)
	{
		screen->markDirty(cursor_t::getArea());
		cursor_t::position = cursor_t::nextPosition;
		screen->markDirty(cursor_t::getArea());
	}

	// set pressed buttons
	cursor_t::pressedButtons = cursor_t::nextPressedButtons;

	mouse_event_t baseEvent;
	baseEvent.screenPosition = cursor_t::position;
	baseEvent.position = baseEvent.screenPosition;
	baseEvent.buttons = cursor_t::pressedButtons;

	// Press
	if((!(previousPressedButtons & G_MOUSE_BUTTON_1) && (cursor_t::pressedButtons & G_MOUSE_BUTTON_1)) ||
	   (!(previousPressedButtons & G_MOUSE_BUTTON_2) && (cursor_t::pressedButtons & G_MOUSE_BUTTON_2)) ||
	   (!(previousPressedButtons & G_MOUSE_BUTTON_3) && (cursor_t::pressedButtons & G_MOUSE_BUTTON_3)))
	{
		// Prepare event
		mouse_event_t pressEvent = baseEvent;
		pressEvent.type = G_MOUSE_EVENT_PRESS;

		// Multiclicks
		static uint64_t lastClick = 0;
		static int clickCount = 0;
		uint64_t currentClick = g_millis();
		uint64_t diff = currentClick - lastClick;
		if(diff < multiclickTimespan)
		{
			++clickCount;
		}
		else
		{
			clickCount = 1;
		}
		lastClick = currentClick;
		pressEvent.clickCount = clickCount;

		// Send event
		auto pressedComponent = instance->dispatch(screen, pressEvent);

		if(pressedComponent)
		{
			// Prepare drag
			cursor_t::pressedComponent = pressedComponent->id;
			cursor_t::draggedComponent = cursor_t::pressedComponent;

			// Switch focus
			if(cursor_t::pressedComponent != cursor_t::focusedComponent)
			{
				auto unfocusedComponent = component_registry_t::get(cursor_t::focusedComponent);
				auto newFocusedComponent = pressedComponent->setFocused(true);

				// New gains focus
				if(newFocusedComponent)
				{
					cursor_t::focusedComponent = newFocusedComponent->id;

					window_t* unfocusedWindow = nullptr;
					// Old loses focus
					if(unfocusedComponent)
					{
						unfocusedComponent->setFocused(false);
						unfocusedWindow = unfocusedComponent->getWindow();
					}

					// Window handling
					window_t* newFocusedWindow = pressedComponent->getWindow();
					if(unfocusedWindow && newFocusedWindow != unfocusedWindow && unfocusedWindow != unfocusedComponent)
					{
						unfocusedWindow->setFocused(false);
					}
					if(newFocusedWindow && newFocusedWindow != newFocusedComponent)
					{
						newFocusedWindow->bringToFront();
						newFocusedWindow->setFocused(true);
					}
				}
			}
		}
		else
		{
			cursor_t::pressedComponent = -1;
		}

		// Release
	}
	else if(
		((previousPressedButtons & G_MOUSE_BUTTON_1) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_1)) ||
		((previousPressedButtons & G_MOUSE_BUTTON_2) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_2)) ||
		((previousPressedButtons & G_MOUSE_BUTTON_3) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_3)))
	{

		if(cursor_t::draggedComponent != -1)
		{
			auto draggedComponent = component_registry_t::get(cursor_t::draggedComponent);

			if(draggedComponent)
			{
				mouse_event_t releaseDraggedEvent = baseEvent;
				releaseDraggedEvent.type = G_MOUSE_EVENT_DRAG_RELEASE;
				instance->dispatchUpwards(draggedComponent, releaseDraggedEvent);
			}

			cursor_t::draggedComponent = -1;
		}

		if(cursor_t::pressedComponent != -1)
		{
			auto pressedComponent = component_registry_t::get(cursor_t::pressedComponent);
			if(pressedComponent)
			{
				mouse_event_t releaseEvent = baseEvent;
				releaseEvent.type = G_MOUSE_EVENT_RELEASE;
				instance->dispatchUpwards(pressedComponent, releaseEvent);
			}

			cursor_t::pressedComponent = -1;
		}

		// Move or drag
	}
	else if(cursor_t::position != previousPosition)
	{
		if(cursor_t::draggedComponent != -1)
		{
			auto draggedComponent = component_registry_t::get(cursor_t::draggedComponent);

			if(draggedComponent)
			{
				// Dragging
				mouse_event_t dragEvent = baseEvent;
				dragEvent.type = G_MOUSE_EVENT_DRAG;
				instance->dispatchUpwards(draggedComponent, dragEvent);
			}
		}
		else
		{
			// Moving
			mouse_event_t moveEvent = baseEvent;
			moveEvent.type = G_MOUSE_EVENT_MOVE;

			component_t* newHoveredComponent = instance->dispatch(screen, moveEvent);

			// Post enter or leave events
			if(newHoveredComponent && (newHoveredComponent->id != cursor_t::hoveredComponent) &&
			   (cursor_t::draggedComponent == -1 || cursor_t::draggedComponent != cursor_t::hoveredComponent))
			{
				// Leave
				if(cursor_t::hoveredComponent != -1)
				{
					auto hoveredComponent = component_registry_t::get(cursor_t::hoveredComponent);

					if(hoveredComponent)
					{
						mouse_event_t leaveEvent = baseEvent;
						leaveEvent.type = G_MOUSE_EVENT_LEAVE;
						instance->dispatchUpwards(hoveredComponent, leaveEvent);
						cursor_t::hoveredComponent = -1;
					}
				}

				if(newHoveredComponent)
				{
					// Enter
					mouse_event_t enterEvent = baseEvent;
					enterEvent.type = G_MOUSE_EVENT_ENTER;
					cursor_t::hoveredComponent = newHoveredComponent->id;
					instance->dispatchUpwards(newHoveredComponent, enterEvent);
				}
			}
		}
	}
}
