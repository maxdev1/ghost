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

#include <components/cursor.hpp>
#include <events/event_processor.hpp>
#include <ghostuser/tasking/lock.hpp>

#include <windowserver.hpp>
#include <components/window.hpp>

#include <events/event.hpp>
#include <events/mouse_event.hpp>
#include <events/key_event.hpp>
#include <events/focus_event.hpp>

static g_lock keyEventQueueLock;
static g_lock mouseEventQueueLock;

/**
 *
 */
event_processor_t::event_processor_t() {
	multiclickTimespan = DEFAULT_MULTICLICK_TIMESPAN;
}

/**
 *
 */
void event_processor_t::queueKeyEvent(g_key_info keyInfo) {
	keyEventQueueLock.lock();
	keyEventQueue.push_back(keyInfo);
	keyEventQueueLock.unlock();

	windowserver_t::instance()->request_step();
}

/**
 *
 */
void event_processor_t::queueMouseEvent(g_mouse_info mouseInfo) {
	mouseEventQueueLock.lock();
	mouseEventQueue.push_back(mouseInfo);
	mouseEventQueueLock.unlock();

	windowserver_t::instance()->request_step();
}

/**
 *
 */
void event_processor_t::process() {

	// dispatch key events
	keyEventQueueLock.lock();
	while (keyEventQueue.size() > 0) {
		translateKeyEvent(keyEventQueue.front());
		keyEventQueue.pop_front();
	}
	keyEventQueueLock.unlock();

	// dispatch mouse events
	mouseEventQueueLock.lock();
	while (mouseEventQueue.size() > 0) {
		translateMouseEvent(mouseEventQueue.front());
		mouseEventQueue.pop_front();
	}
	mouseEventQueueLock.unlock();

	// TODO dispatch command events
}

/**
 *
 */
void event_processor_t::translateKeyEvent(g_key_info& info) {

	if (cursor_t::focusedComponent) {
		key_event_t k;
		k.info = info;
		windowserver_t::instance()->dispatch(cursor_t::focusedComponent, k);
	}
}

/**
 *
 */
void event_processor_t::translateMouseEvent(g_mouse_info& info) {

	g_point previousPosition = cursor_t::position;
	mouse_button_t previousPressedButtons = cursor_t::pressedButtons;

	g_dimension resolution = windowserver_t::instance()->video_output->getResolution();
	windowserver_t* instance = windowserver_t::instance();
	screen_t* screen = instance->screen;

	// Invalidate old location
	screen->markDirty(cursor_t::getArea());

	// Calculate new cursor position
	cursor_t::position.x += info.x;
	cursor_t::position.y -= info.y;

	if (cursor_t::position.x < 0) {
		cursor_t::position.x = 0;
	}
	if (cursor_t::position.x > resolution.width - 2) {
		cursor_t::position.x = resolution.width - 2;
	}
	if (cursor_t::position.y < 0) {
		cursor_t::position.y = 0;
	}
	if (cursor_t::position.y > resolution.height - 2) {
		cursor_t::position.y = resolution.height - 2;
	}

	// Invalidate new location
	screen->markDirty(cursor_t::getArea());

	// Set pressed buttons
	cursor_t::pressedButtons = MOUSE_BUTTON_NONE;
	if (info.button1) {
		cursor_t::pressedButtons |= MOUSE_BUTTON_1;
	}
	if (info.button2) {
		cursor_t::pressedButtons |= MOUSE_BUTTON_2;
	}
	if (info.button3) {
		cursor_t::pressedButtons |= MOUSE_BUTTON_3;
	}

	mouse_event_t baseEvent;
	baseEvent.screenPosition = cursor_t::position;
	baseEvent.position = baseEvent.screenPosition;
	baseEvent.buttons = cursor_t::pressedButtons;

	// Press
	if ((!(previousPressedButtons & MOUSE_BUTTON_1) && (cursor_t::pressedButtons & MOUSE_BUTTON_1))
			|| (!(previousPressedButtons & MOUSE_BUTTON_2) && (cursor_t::pressedButtons & MOUSE_BUTTON_2))
			|| (!(previousPressedButtons & MOUSE_BUTTON_3) && (cursor_t::pressedButtons & MOUSE_BUTTON_3))) {

		// Prepare event
		mouse_event_t pressEvent = baseEvent;
		pressEvent.type = MOUSE_EVENT_PRESS;

		// Multiclicks
		static uint64_t lastClick = 0;
		static int clickCount = 0;
		uint64_t currentClick = g_millis();
		uint64_t diff = currentClick - lastClick;
		if (diff < multiclickTimespan) {
			++clickCount;
		} else {
			clickCount = 1;
		}
		lastClick = currentClick;
		pressEvent.clickCount = clickCount;

		// Send event
		instance->dispatch(screen, pressEvent);

		component_t* hitComponent = screen->getComponentAt(cursor_t::position);
		if (hitComponent != 0) {

			// Prepare drag
			if (hitComponent != screen) {
				cursor_t::draggedComponent = hitComponent;
			}

			// Switch focus
			if (hitComponent != cursor_t::focusedComponent) {
				// Old loses focus
				if (cursor_t::focusedComponent != 0) {
					focus_event_t focusLostEvent;
					focusLostEvent.type = FOCUS_EVENT_LOST;
					instance->dispatchUpwards(cursor_t::focusedComponent, focusLostEvent);
				}

				// Bring hit components window to front
				window_t* parentWindow = hitComponent->getWindow();
				if (parentWindow != 0) {
					parentWindow->bringToFront();
				}

				// New gains focus
				focus_event_t focusGainedEvent;
				focusGainedEvent.type = FOCUS_EVENT_GAINED;
				cursor_t::focusedComponent = instance->dispatchUpwards(hitComponent, focusGainedEvent);
			}
		}

		// Release
	} else if (((previousPressedButtons & MOUSE_BUTTON_1) && !(cursor_t::pressedButtons & MOUSE_BUTTON_1))
			|| ((previousPressedButtons & MOUSE_BUTTON_2) && !(cursor_t::pressedButtons & MOUSE_BUTTON_2))
			|| ((previousPressedButtons & MOUSE_BUTTON_3) && !(cursor_t::pressedButtons & MOUSE_BUTTON_3))) {

		if (cursor_t::draggedComponent) {
			mouse_event_t releaseDraggedEvent = baseEvent;
			releaseDraggedEvent.type = MOUSE_EVENT_DRAG_RELEASE;
			instance->dispatchUpwards(cursor_t::draggedComponent, releaseDraggedEvent);
			cursor_t::draggedComponent = 0;
		}

		mouse_event_t releaseEvent = baseEvent;
		releaseEvent.type = MOUSE_EVENT_RELEASE;
		instance->dispatch(screen, releaseEvent);

		// Move or drag
	} else if (cursor_t::position != previousPosition) {

		component_t* hovered = screen->getComponentAt(cursor_t::position);
		if (hovered != 0 && (hovered != cursor_t::hoveredComponent)) {

			// Leave
			if (cursor_t::hoveredComponent) {
				mouse_event_t leaveEvent = baseEvent;
				leaveEvent.type = MOUSE_EVENT_LEAVE;
				instance->dispatchUpwards(cursor_t::hoveredComponent, leaveEvent);
				cursor_t::hoveredComponent = 0;
			}

			// Enter
			mouse_event_t enterEvent = baseEvent;
			enterEvent.type = MOUSE_EVENT_ENTER;
			cursor_t::hoveredComponent = hovered;
			instance->dispatchUpwards(cursor_t::hoveredComponent, enterEvent);
		}

		if (cursor_t::draggedComponent != 0) { // Dragging
			mouse_event_t dragEvent = baseEvent;
			dragEvent.type = MOUSE_EVENT_DRAG;
			instance->dispatchUpwards(cursor_t::draggedComponent, dragEvent);

		} else { // Moving
			mouse_event_t moveEvent = baseEvent;
			moveEvent.type = MOUSE_EVENT_MOVE;
			instance->dispatch(screen, moveEvent);
		}
	}
}
