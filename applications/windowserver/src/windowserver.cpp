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
#include "components/button.hpp"
#include "components/cursor.hpp"
#include "components/desktop/background.hpp"
#include "components/window.hpp"
#include "events/event.hpp"
#include "events/locatable.hpp"
#include "input/input_receiver.hpp"
#include "interface/interface_responder.hpp"
#include "interface/registration_thread.hpp"
#include "test.hpp"
#include "video/vbe_video_output.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <iostream>
#include <libproperties/parser.hpp>
#include <stdio.h>
#include <typeinfo>

static windowserver_t* server;
static g_atom dispatch_lock = g_atomic_initialize();
static int framesTotal = 0;

int main(int argc, char** argv)
{
	server = new windowserver_t();
	server->launch();
	return 0;
}

windowserver_t* windowserver_t::instance()
{
	return server;
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
	event_processor = new event_processor_t();

	g_task_register_id("windowserver");
	initializeGraphics();
	g_create_thread((void*) &windowserver_t::initializeInput);

	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	createVitalComponents(screenBounds);

	createInterface();

	// test_t::createTestComponents();
	// g_spawn("/applications/terminal.bin", "", "", G_SECURITY_LEVEL_APPLICATION);

	renderLoop(screenBounds);
}

void windowserver_t::createVitalComponents(g_rectangle screenBounds)
{
	screen = new screen_t();
	screen->setBounds(screenBounds);

	background = new background_t();
	background->setBounds(screenBounds);
	screen->addChild(background);
	cursor_t::focusedComponent = screen;
	background->startLoadDesktopItems();

	stateLabel = new label_t();
	stateLabel->setTitle("");
	stateLabel->setAlignment(g_text_alignment::RIGHT);
	stateLabel->setBounds(g_rectangle(screenBounds.width - 200, screenBounds.height - 30, 200, 30));
	background->addChild(stateLabel);

	// background.load("/system/graphics/wallpaper.png");
}

void windowserver_t::initializeGraphics()
{
	g_set_video_log(false);

	video_output = new g_vbe_video_output();
	if(!video_output->initialize())
	{
		std::cerr << "failed to initialize video mode" << std::endl;
		klog("failed to initialize video mode");
		return;
	}
}

void windowserver_t::renderLoop(g_rectangle screenBounds)
{
	g_create_thread((void*) &windowserver_t::fpsCounter);
	g_task_register_id("windowserver/renderer");

	g_graphics global;
	global.resize(screenBounds.width, screenBounds.height, false);

	cursor_t::nextPosition = g_point(screenBounds.width / 2, screenBounds.height / 2);

	g_atomic_lock(render_atom);
	while(true)
	{
		event_processor->process();

		screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT);

		screen->blit(&global, screenBounds, g_point(0, 0));
		cursor_t::paint(&global);

		blit(&global);

		framesTotal++;
		g_atomic_lock_to(render_atom, 100);
	}
}

void windowserver_t::triggerRender()
{
	g_atomic_unlock(render_atom);
}

void windowserver_t::blit(g_graphics* graphics)
{
	g_rectangle invalid = screen->grabInvalid();
	if(invalid.width == 0 && invalid.height == 0)
		return;

	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	g_color_argb* buffer = (g_color_argb*) cairo_image_surface_get_data(graphics->getSurface());
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
	while(dispatch(acceptor, event) == nullptr)
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

component_t* windowserver_t::dispatch(component_t* component, event_t& event)
{
	component_t* handledBy = nullptr;

	if(component->canHandleEvents())
	{
		locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
		if(locatable != 0)
		{
			g_point locationOnScreen = component->getLocationOnScreen();
			locatable->position.x -= locationOnScreen.x;
			locatable->position.y -= locationOnScreen.y;
		}

		g_atomic_lock(dispatch_lock);
		handledBy = event.visit(component);
		g_atomic_unlock(dispatch_lock);
	}

	return handledBy;
}

void windowserver_t::fpsCounter()
{
	g_task_register_id("windowserver/fps-counter");
	int seconds = 0;

	int renders = 0;
	for(;;)
	{
		g_sleep(1000);
		seconds++;
		std::stringstream s;
		s << "FPS: " << framesTotal << ", Time: " << renders++ << "s";
		windowserver_t::instance()->stateLabel->setTitle(s.str());
		windowserver_t::instance()->triggerRender();
		framesTotal = 0;
	}
}

void windowserver_t::createInterface()
{
	g_create_thread((void*) &interfaceRegistrationThread);
	g_create_thread((void*) &interfaceResponderThread);
}
