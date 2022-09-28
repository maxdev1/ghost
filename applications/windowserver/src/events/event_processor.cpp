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
#include "components/button.hpp"
#include "components/cursor.hpp"
#include "components/label.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "events/event.hpp"
#include "events/focus_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"
#include "input/input_receiver.hpp"
#include "windowserver.hpp"

event_processor_t::event_processor_t()
{
	multiclickTimespan = DEFAULT_MULTICLICK_TIMESPAN;
}

void event_processor_t::bufferKeyEvent(g_key_info keyInfo)
{
	g_atomic_lock(&key_info_buffer_lock);
	key_info_buffer.push_back(keyInfo);
	key_info_buffer_lock = 0;
}

void event_processor_t::bufferCommandMessage(void* commandMessage)
{
	g_atomic_lock(&command_message_buffer_lock);
	command_message_buffer.push_back(commandMessage);
	command_message_buffer_lock = 0;
}

void event_processor_t::process()
{
	processMouseState();
	processKeyState();
}

void event_processor_t::processKeyState()
{
	g_atomic_lock(&key_info_buffer_lock);
	while(key_info_buffer.size() > 0)
	{
		translateKeyEvent(key_info_buffer.back());
		key_info_buffer.pop_back();
	}
	key_info_buffer_lock = 0;
}

void event_processor_t::translateKeyEvent(g_key_info& info)
{
	if(cursor_t::focusedComponent)
	{
		// process
		key_event_t k;
		k.info = info;
		windowserver_t::instance()->dispatch(cursor_t::focusedComponent, k);
	}
}

void event_processor_t::processMouseState()
{
	g_point previousPosition = cursor_t::position;
	g_mouse_button previousPressedButtons = cursor_t::pressedButtons;

	g_dimension resolution = windowserver_t::instance()->video_output->getResolution();
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
		cursor_t::pressedComponent = instance->dispatch(screen, pressEvent);

		if(cursor_t::pressedComponent)
		{
			// Prepare drag
			cursor_t::draggedComponent = cursor_t::pressedComponent;

			// Switch focus
			if(cursor_t::pressedComponent != cursor_t::focusedComponent)
			{
				// Old loses focus
				if(cursor_t::focusedComponent)
				{
					focus_event_t focusLostEvent;
					focusLostEvent.type = FOCUS_EVENT_LOST;
					focusLostEvent.newFocusedComponent = cursor_t::pressedComponent;
					instance->dispatchUpwards(cursor_t::focusedComponent, focusLostEvent);

					// Post event to client
					event_listener_info_t listenerInfo;
					if(cursor_t::focusedComponent->getListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, listenerInfo))
					{
						g_ui_component_focus_event focus_event;
						focus_event.header.type = G_UI_COMPONENT_EVENT_TYPE_FOCUS;
						focus_event.header.component_id = listenerInfo.component_id;
						focus_event.now_focused = false;
						g_send_message(listenerInfo.target_thread, &focus_event, sizeof(g_ui_component_focus_event));
					}
				}

				// Bring hit components window to front
				window_t* parentWindow = cursor_t::pressedComponent->getWindow();
				if(parentWindow != 0)
				{
					parentWindow->bringToFront();
				}

				// New gains focus
				focus_event_t focusGainedEvent;
				focusGainedEvent.type = FOCUS_EVENT_GAINED;
				focusGainedEvent.newFocusedComponent = cursor_t::pressedComponent;
				cursor_t::focusedComponent = instance->dispatchUpwards(cursor_t::pressedComponent, focusGainedEvent);

				// Post event to client
				event_listener_info_t listenerInfo;
				if(cursor_t::focusedComponent && cursor_t::focusedComponent->getListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, listenerInfo))
				{
					g_ui_component_focus_event focus_event;
					focus_event.header.type = G_UI_COMPONENT_EVENT_TYPE_FOCUS;
					focus_event.header.component_id = listenerInfo.component_id;
					focus_event.now_focused = true;
					g_send_message(listenerInfo.target_thread, &focus_event, sizeof(g_ui_component_focus_event));
				}
			}
		}

		// Release
	}
	else if(
		((previousPressedButtons & G_MOUSE_BUTTON_1) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_1)) ||
		((previousPressedButtons & G_MOUSE_BUTTON_2) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_2)) ||
		((previousPressedButtons & G_MOUSE_BUTTON_3) && !(cursor_t::pressedButtons & G_MOUSE_BUTTON_3)))
	{

		if(cursor_t::draggedComponent)
		{
			mouse_event_t releaseDraggedEvent = baseEvent;
			releaseDraggedEvent.type = G_MOUSE_EVENT_DRAG_RELEASE;
			instance->dispatchUpwards(cursor_t::draggedComponent, releaseDraggedEvent);

			cursor_t::draggedComponent = 0;
		}

		if(cursor_t::pressedComponent)
		{
			mouse_event_t releaseEvent = baseEvent;
			releaseEvent.type = G_MOUSE_EVENT_RELEASE;
			instance->dispatchUpwards(cursor_t::pressedComponent, releaseEvent);

			cursor_t::pressedComponent = 0;
		}

		// Move or drag
	}
	else if(cursor_t::position != previousPosition)
	{
		// Post enter or leave events
		component_t* hovered = screen->getComponentAt(cursor_t::position);
		if((hovered != cursor_t::hoveredComponent) &&
		   (cursor_t::draggedComponent == 0 || cursor_t::draggedComponent != cursor_t::hoveredComponent))
		{

			// Leave
			if(cursor_t::hoveredComponent)
			{
				mouse_event_t leaveEvent = baseEvent;
				leaveEvent.type = G_MOUSE_EVENT_LEAVE;
				instance->dispatchUpwards(cursor_t::hoveredComponent, leaveEvent);
				cursor_t::hoveredComponent = 0;
			}

			if(hovered)
			{
				// Enter
				mouse_event_t enterEvent = baseEvent;
				enterEvent.type = G_MOUSE_EVENT_ENTER;
				cursor_t::hoveredComponent = hovered;
				instance->dispatchUpwards(cursor_t::hoveredComponent, enterEvent);
			}
		}

		if(cursor_t::draggedComponent != 0)
		{ // Dragging
			mouse_event_t dragEvent = baseEvent;
			dragEvent.type = G_MOUSE_EVENT_DRAG;
			instance->dispatchUpwards(cursor_t::draggedComponent, dragEvent);
		}
		else
		{ // Moving
			mouse_event_t moveEvent = baseEvent;
			moveEvent.type = G_MOUSE_EVENT_MOVE;
			instance->dispatch(screen, moveEvent);
		}
	}
}
