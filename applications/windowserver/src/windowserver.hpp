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

#ifndef __WINDOWSERVER__
#define __WINDOWSERVER__

#include <components/component.hpp>
#include <components/screen.hpp>
#include <components/label.hpp>
#include <events/event_processor.hpp>
#include "output/video_output.hpp"
#include "interface/command_message_responder_thread.hpp"

#define BENCHMARKING 0

/**
 *
 */
class windowserver_t {
public:
	video_output_t* video_output;
	event_processor_t* event_processor;
	screen_t* screen;
	command_message_responder_thread_t* responder_thread;
	uint8_t render_atom;

	/**
	 * Sets up the windowing system by configuring a video output, setting up the
	 * event processor and running the main loop. Each step of the main loop includes
	 * a event handling and rendering sequence.
	 */
	void launch();

	/**
	 *
	 */
	void mainLoop(g_rectangle screenBounds);

	/**
	 * Blits the component state.
	 */
	void blit(g_graphics* graphics);

	/**
	 * Dispatches the given event to the component.
	 *
	 * @return whether the event was handled
	 */
	bool dispatch(component_t* component, event_t& event);

	/**
	 * Dispatches the given event upwards the component tree.
	 */
	component_t* dispatchUpwards(component_t* component, event_t& event);

	/**
	 * Returns the singleton instance of the window server.
	 *
	 * @return the instance
	 */
	static windowserver_t* instance();

	/**
	 * TODO remove
	 */
	void createTestComponents();

	/**
	 *
	 */
	void loadCursor();

	/**
	 *
	 */
	void triggerRender();

};

#endif
