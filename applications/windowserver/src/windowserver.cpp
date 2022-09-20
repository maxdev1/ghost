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

#include "windowserver.hpp"
#include "components/background.hpp"
#include "components/button.hpp"
#include "components/checkbox.hpp"
#include "components/cursor.hpp"
#include "components/plain_console_panel.hpp"
#include "components/scrollbar.hpp"
#include "components/scrollpane.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "events/event.hpp"
#include "events/locatable.hpp"
#include "input/input_receiver.hpp"
#include "layout/flow_layout_manager.hpp"
#include "layout/grid_layout_manager.hpp"
#include "video/vbe_video_output.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <iostream>
#include <libproperties/parser.hpp>
#include <stdio.h>
#include <typeinfo>

static windowserver_t* server;
static uint8_t dispatch_lock = 0;
static int framesTotal = 0;

int main(int argc, char** argv)
{
    server = new windowserver_t();
    server->launch();
    return 0;
}

void windowserver_t::initializeInput()
{
    input_receiver_t::initialize();

    std::string keyLayout = "de-DE";
    if(!g_keyboard::loadLayout(keyLayout))
    {
        klog(("failed to load keyboard layout '" + keyLayout + "', no keyboard input available").c_str());
    }

    server->loadCursor();
    server->triggerRender();
}

void windowserver_t::launch()
{
    g_task_register_id("windowserver");
    g_set_video_log(false);

    // initialize the video output
    video_output = new g_vbe_video_output();
    if(!video_output->initialize())
    {
        std::cerr << "failed to initialize video mode" << std::endl;
        klog("failed to initialize video mode");
        return;
    }

    // set up event handling
    event_processor = new event_processor_t();
    g_create_thread((void*) &windowserver_t::initializeInput);

    g_dimension resolution = video_output->getResolution();
    g_rectangle screenBounds(0, 0, resolution.width, resolution.height);

    screen = new screen_t();
    screen->setBounds(screenBounds);

    background_t background(screenBounds);
    screen->addChild(&background);
    cursor_t::focusedComponent = screen;

    // background.load("/system/graphics/wallpaper.png");

    createTestComponents();

    mainLoop(screenBounds);
}

void windowserver_t::fpsCounter()
{
    int seconds = 0;

    for(;;)
    {
        g_sleep(1000);
        seconds++;
        klog("fps: %i", framesTotal);
        framesTotal = 0;
    }
}

void windowserver_t::mainLoop(g_rectangle screenBounds)
{
    g_create_thread((void*) &windowserver_t::fpsCounter);

    g_graphics global;
    global.resize(screenBounds.width, screenBounds.height);

    cursor_t::nextPosition = g_point(screenBounds.width / 2, screenBounds.height / 2);

    render_atom = true;
    while(true)
    {
        event_processor->processMouseState();
        event_processor->process();

        screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE);
        screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT);
        screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT);

        screen->blit(&global, screenBounds, g_point(0, 0));
        cursor_t::paint(&global);

        blit(&global);

        framesTotal++;
        g_atomic_lock_to(&render_atom, 1000);
    }
}

void windowserver_t::blit(g_graphics* graphics)
{

    g_dimension resolution = video_output->getResolution();
    g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
    g_color_argb* buffer = (g_color_argb*) cairo_image_surface_get_data(graphics->getSurface());

    g_rectangle invalid = screen->grabInvalid();
    if(invalid.width == 0 && invalid.height == 0)
    {
        return;
    }

    video_output->blit(invalid, screenBounds, buffer);
}

void windowserver_t::loadCursor()
{
    cursor_t::load("/system/graphics/cursor/default.cursor");
    cursor_t::load("/system/graphics/cursor/text.cursor");
    cursor_t::load("/system/graphics/cursor/resize-ns.cursor");
    cursor_t::load("/system/graphics/cursor/resize-ew.cursor");
    cursor_t::load("/system/graphics/cursor/resize-nesw.cursor");
    cursor_t::load("/system/graphics/cursor/resize-nwes.cursor");
    cursor_t::set("default");
}

class open_executable_action_handler_t;
void open_executable_spawn(open_executable_action_handler_t* data);

class open_executable_action_handler_t : public internal_action_handler_t
{
  public:
    std::string exe;
    std::string args;
    open_executable_action_handler_t(std::string exe, std::string args) : exe(exe), args(args)
    {
    }
    void handle(action_component_t* source)
    {
        g_create_thread_d((void*) &open_executable_spawn, this);
    }
};

void open_executable_spawn(open_executable_action_handler_t* data)
{
    g_spawn(data->exe.c_str(), data->args.c_str(), "/", G_SECURITY_LEVEL_APPLICATION);
}

static int nextExeButtonPos = 70;
void addExecutableButton(window_t* window, std::string name, std::string exe, std::string args)
{

    button_t* openCalculatorButton = new button_t();
    openCalculatorButton->setBounds(g_rectangle(10, nextExeButtonPos, 270, 30));
    openCalculatorButton->getLabel().setTitle(name);
    openCalculatorButton->setInternalActionHandler(new open_executable_action_handler_t(exe, args));
    window->addChild(openCalculatorButton);
    nextExeButtonPos += 35;
}

void windowserver_t::createTestComponents()
{

    window_t* testWindow = new window_t;
    testWindow->setTitle("Welcome to Ghost!");
    testWindow->setBounds(g_rectangle(10, 10, 320, 230));
    // testWindow->setNumericProperty(G_UI_PROPERTY_RESIZABLE, false);

    label_t* infoLabel = new label_t();
    infoLabel->setBounds(g_rectangle(10, 10, 300, 20));
    infoLabel->setTitle("This is a demo launchpad for executing");
    infoLabel->setColor(RGB(0, 0, 0));
    testWindow->addChild(infoLabel);

    label_t* infoLabel2 = new label_t();
    infoLabel2->setBounds(g_rectangle(10, 30, 300, 20));
    infoLabel2->setTitle("the available GUI applications.");
    infoLabel2->setColor(RGB(0, 0, 0));
    testWindow->addChild(infoLabel2);

    addExecutableButton(testWindow, "Calculator", "/applications/calculator.bin", "");
    addExecutableButton(testWindow, "Terminal", "/applications/terminal2.bin", "");
    addExecutableButton(testWindow, "Drawing demo", "/applications/tetris.bin", "");

    text_field_t* testField = new text_field_t();
    testField->setBounds(g_rectangle(10, nextExeButtonPos, 270, 30));
    testWindow->addChild(testField);

    screen->addChild(testWindow);
    testWindow->setVisible(true);

    window_t* secondWindow = new window_t;
    secondWindow->setTitle("Scroller");
    secondWindow->setBounds(g_rectangle(200, 200, 500, 500));
    secondWindow->setLayoutManager(new grid_layout_manager_t(1, 1));

    scrollpane_t* scroller = new scrollpane_t;
    scroller->setBounds(g_rectangle(0, 0, 300, 200));
    secondWindow->addChild(scroller);

    panel_t* contentPanel = new panel_t();
    contentPanel->setBounds(g_rectangle(0, 0, 400, 400));
    contentPanel->setBackground(RGB(200, 200, 200));
    contentPanel->setLayoutManager(new grid_layout_manager_t(1, 5));
    scroller->setViewPort(contentPanel);

    /*   button_t* button1 = new button_t();
       button1->getLabel().setTitle("Button 1");
       contentPanel->addChild(button1);

       label_t* label1 = new label_t();
       label1->setTitle("This is a panel with some scrollable content. The content is layouted using a grid layout.");
       contentPanel->addChild(label1);

       label_t* label2 = new label_t();
       label2->setTitle("The height of the content panel is used to specify the scrollable area.");
       contentPanel->addChild(label2);

       button_t* button2 = new button_t();
       button2->getLabel().setTitle("Button 2");
       contentPanel->addChild(button2);

       label_t* label3 = new label_t();
       label3->setTitle("Yey! Works great :)");
       contentPanel->addChild(label3);*/

    screen->addChild(secondWindow);
    secondWindow->setVisible(true);
}

component_t* windowserver_t::dispatchUpwards(component_t* component, event_t& event)
{

    // store when dispatching to parents
    g_point initialPosition;
    locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
    if(locatable)
    {
        initialPosition = locatable->position;
    }

    // check upwards until someone accepts the event
    component_t* acceptor = component;
    while(!dispatch(acceptor, event))
    {
        acceptor = acceptor->getParent();
        if(acceptor == 0)
        {
            break;
        }

        // restore position on locatable events
        if(locatable)
        {
            locatable->position = initialPosition;
        }
    }
    return acceptor;
}

bool windowserver_t::dispatch(component_t* component, event_t& event)
{
    g_atomic_lock(&dispatch_lock);

    bool handled = false;

    if(component->canHandleEvents())
    {
        locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
        if(locatable != 0)
        {
            g_point locationOnScreen = component->getLocationOnScreen();
            locatable->position.x -= locationOnScreen.x;
            locatable->position.y -= locationOnScreen.y;
        }

        handled = component->handle(event);
    }

    dispatch_lock = 0;
    return handled;
}

windowserver_t* windowserver_t::instance()
{
    return server;
}

void windowserver_t::triggerRender()
{
    render_atom = false;
}
