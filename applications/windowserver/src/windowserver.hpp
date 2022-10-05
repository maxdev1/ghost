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

#ifndef __WINDOWSERVER__
#define __WINDOWSERVER__

#include "components/component.hpp"
#include "components/desktop/background.hpp"
#include "components/desktop/screen.hpp"
#include "components/label.hpp"
#include "events/event_processor.hpp"
#include "video/video_output.hpp"

/**
 *
 */
class windowserver_t
{
  public:
	g_video_output* video_output;
	event_processor_t* event_processor;
	screen_t* screen;
	background_t* background;
	label_t* stateLabel;
	g_atom render_atom = g_atomic_initialize();

	/**
	 * Sets up the windowing system by configuring a video output, setting up the
	 * event processor and running the main loop. Each step of the main loop includes
	 * a event handling and rendering sequence.
	 */
	void launch();

	void initializeGraphics();
	void createVitalComponents(g_rectangle screenBounds);
	void loadCursor();

	void renderLoop(g_rectangle screenBounds);
	void triggerRender();
	static void initializeInput();
	static void fpsCounter();

	/**
	 * Blits the component state.
	 */
	void blit(g_graphics* graphics);

	/**
	 * Dispatches the given event to the component.
	 *
	 * @return the component that has handled the event or NULL
	 */
	component_t* dispatch(component_t* component, event_t& event);

	/**
	 * Dispatches the given event upwards the component tree.
	 */
	component_t* dispatchUpwards(component_t* component, event_t& event);

	/**
	 * Creates the command receiver interface.
	 */
	void createInterface();

	/**
	 * Returns the singleton instance of the window server.
	 *
	 * @return the instance
	 */
	static windowserver_t* instance();
};

#endif
