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
#include "components/desktop/screen.hpp"
#include "components/label.hpp"
#include "events/event_processor.hpp"
#include "video/video_output.hpp"

class item_container_t;

/**
 *
 */
class windowserver_t
{
    g_tid updateTask = G_TID_NONE;
    g_tid renderTask = G_TID_NONE;

    g_user_mutex updateLock = g_mutex_initialize();
    g_user_mutex lazyUpdateLock = g_mutex_initialize();

    void initializeVideo();
    void createVitalComponents(g_rectangle screenBounds);
    g_video_output* findVideoOutput();
    void loadCursor();
    static void startInputHandlers();

    void updateLoop(const g_rectangle& screenBounds) const;
    void updateDebounceLoop() const;

    static void fpsCounter();

public:
    g_video_output* videoOutput;
    event_processor_t* eventProcessor;
    screen_t* screen;
    label_t* stateLabel;

    windowserver_t();

    /**
     * Sets up the windowing system by configuring a video output, setting up
     * the event processor and running the main loop. Each step of the main loop
     * includes an event handling and rendering sequence.
     */
    void launch();

    /**
     * When components have changed, requests the server to perform an update of
     * all components. This triggers the update to run immediately.
     */
    void requestUpdateImmediately() const;

    /**
     * Also requests an update, but debounces requests that come in very quickly.
     * This is used to avoid that for example a large amount of changes via the
     * interface cause too many updates.
     */
    void requestUpdateLater() const;

    static void setDebug(bool cond);
    static bool isDebug();

    /**
     * Blits the component state.
     */
    void output(graphics_t* graphics) const;

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
     * Switches the focus from the currently focused component to a different
     * component. Must be used to ensure state is updated correctly.
     */
    component_t* switchFocus(component_t* to);

    /**
     * Returns the singleton instance of the window server.
     *
     * @return the instance
     */
    static windowserver_t* instance();

    static void startLazyUpdateLoop();
    static void startOtherTasks();
};

#endif
