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
#include "component_registry.hpp"
#include "components/button.hpp"
#include "components/cursor.hpp"
#include "events/event.hpp"
#include "events/locatable.hpp"
#include "interface/registration_thread.hpp"
#include "components/window.hpp"
#include "components/text/text_field.hpp"
#include "test.hpp"
#include "platform/platform.hpp"

#include <cairo/cairo.h>
#include <iostream>
#include <cstdio>
#include <sstream>

#ifdef _GHOST_
#include <libdevice/interface.hpp>
#endif

static SYS_MUTEX_T dispatchLock = platformInitializeMutex(true);
static int framesTotal = 0;
static bool debugOn = false;
int debugBorderCycle = 0;

windowserver_t* windowserver_t::instance()
{
	return server;
}

windowserver_t::windowserver_t()
{
	eventProcessor = new event_processor_t();
}

void windowserver_t::startInputHandlers()
{
	platformStartInput();

	std::string keyLayout = "de-DE";
	if(!platformInitializeKeyboardLayout(keyLayout))
	{
		platformLog(("failed to load keyboard layout '" + keyLayout + "', no keyboard input available").c_str());
	}

	platformLoadCursors();
	server->requestUpdateLater();
}

void windowserver_t::startOtherTasks()
{
	platformRegisterTaskIdentifier("windowserver/launcher");
	// TODO not the windowservers job
	platformSpawn("/applications/desktop.bin", "", "");
}

void windowserver_t::startLazyUpdateLoop()
{
	windowserver_t::instance()->updateDebounceLoop();
}

void windowserver_t::launch()
{
	platformRegisterTaskIdentifier("windowserver");

	initializeVideo();

	platformCreateThread((void*) &startInputHandlers);

	g_dimension resolution = videoOutput->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	createVitalComponents(screenBounds);

	test_t::createTestComponents();

	platformCreateThread((void*) &interfaceRegistrationThread);
	platformCreateThread((void*) &startOtherTasks);
	platformCreateThread((void*) &startLazyUpdateLoop);

	renderTask = platformGetTid();
	updateLoop(screenBounds);
}

void windowserver_t::createVitalComponents(g_rectangle screenBounds)
{
	screen = new screen_t();
	screen->setBounds(screenBounds);

	stateLabel = new label_t();
	stateLabel->setTitle("fps");
	stateLabel->setAlignment(g_text_alignment::RIGHT);
	stateLabel->setVisible(debugOn);
	stateLabel->setBounds(g_rectangle(10, screenBounds.height - 100, screenBounds.width - 20, 30));
	instance()->stateLabel->setColor(RGB(255, 255, 255));
	screen->addChild(stateLabel);

#ifndef _GHOST_
	panel_t* backgroundPanel = new panel_t();
	backgroundPanel->setBounds(screenBounds);
	backgroundPanel->setBackground(RGB(0,0,0));
	screen->addChild(backgroundPanel);
#endif

	platformCreateThread((void*) &windowserver_t::fpsCounter);
}

void windowserver_t::initializeVideo()
{
	videoOutput = platformCreateVideoOutput();

	if(!videoOutput->initialize())
	{
		platformLog("failed to initialize generic video output");
		platformExit(-1);
	}
}

void windowserver_t::updateLoop(const g_rectangle& screenBounds)
{
	platformRegisterTaskIdentifier("windowserver/updater");

	graphics_t global;
	global.resize(screenBounds.width, screenBounds.height, false);

	cursor_t::nextPosition = g_point(screenBounds.width / 2, screenBounds.height / 2);

	uint64_t lastUpdate = 0;
	platformAcquireMutex(updateLock);
	while(true)
	{
		eventProcessor->process();

		screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE, 0);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT, 0);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT, 0);

		screen->blit(&global, screenBounds, g_point(0, 0));
		cursor_t::paint(&global);

		output(&global);

		framesTotal++;
		platformAcquireMutexTimeout(updateLock, 1000);

		auto now = platformMillis();
		if(now - lastUpdate < 10)
			platformSleep(5);
		lastUpdate = now;
	}
}

void windowserver_t::requestUpdateImmediately() const
{
	platformReleaseMutex(updateLock);
}

void windowserver_t::requestUpdateLater() const
{
	platformReleaseMutex(lazyUpdateLock);
}

void windowserver_t::updateDebounceLoop() const
{
	platformAcquireMutex(lazyUpdateLock);
	while(true)
	{
		platformSleep(10);
		platformAcquireMutexTimeout(lazyUpdateLock, 1000);
		platformReleaseMutex(updateLock);
	}
}

void windowserver_t::output(graphics_t* graphics)
{
	g_rectangle invalid = screen->grabInvalid();
	if(invalid.width == 0 && invalid.height == 0)
		return;

	// Draw the green debug border around invalid area
	if(debugOn)
	{
		auto cr = graphics->acquireContext();
		if(cr)
		{
			debugBorderCycle++;
			if(debugBorderCycle > 100)
				debugBorderCycle = 0;

			cairo_save(cr);
			cairo_set_line_width(cr, 2);
			cairo_rectangle(cr, invalid.x, invalid.y, invalid.width, invalid.height);
			cairo_set_source_rgba(cr,
			                      debugBorderCycle % 3 == 0 ? ((double) (debugBorderCycle + 100)) / 255 : 0,
			                      debugBorderCycle % 2 == 0 ? ((double) (debugBorderCycle + 100)) / 255 : 0,
			                      debugBorderCycle % 2 == 1 ? ((double) (debugBorderCycle + 100)) / 255 : 0,
			                      1);
			cairo_stroke(cr);
			cairo_restore(cr);
			graphics->releaseContext();
		}
	}

	g_dimension resolution = videoOutput->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);

	auto screenSurface = graphics->getSurface();
	cairo_surface_flush(screenSurface);
	auto buffer = (g_color_argb*) cairo_image_surface_get_data(screenSurface);

	g_rectangle totalInvalid = invalid;
	totalInvalid.extend(lastInvalid.getStart());
	totalInvalid.extend(lastInvalid.getEnd());
	videoOutput->blit(totalInvalid.clip(screenBounds), screenBounds, buffer);

	lastInvalid = invalid;
}

component_t* windowserver_t::dispatchUpwards(component_t* component, event_t& event)
{
	// store when dispatching to parents
	g_point initialPosition;
	locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
	if(locatable)
		initialPosition = locatable->position;

	// check upwards until someone accepts the event
	component_t* acceptor = component;
	while(dispatch(acceptor, event) == nullptr)
	{
		acceptor = acceptor->getParent();
		if(acceptor == nullptr)
			break;

		// restore position on locatable events
		if(locatable)
			locatable->position = initialPosition;
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

		platformAcquireMutex(dispatchLock);
		handledBy = event.visit(component);
		platformReleaseMutex(dispatchLock);
	}

	return handledBy;
}

component_t* windowserver_t::switchFocus(component_t* to)
{
	auto from = component_registry_t::get(cursor_t::focusedComponent);

	auto actualTo = to->setFocused(true);
	if(actualTo)
	{
		cursor_t::focusedComponent = actualTo->id;

		window_t* fromWindow = nullptr;
		if(from && from != actualTo)
		{
			from->setFocused(false);
			fromWindow = from->getWindow();
		}

		window_t* toWindow = actualTo->getWindow();
		if(fromWindow && toWindow != fromWindow && fromWindow != from)
		{
			fromWindow->setFocused(false);
		}

		if(toWindow)
		{
			toWindow->bringToFront();
			if(toWindow != actualTo)
			{
				toWindow->setFocused(true);
			}
		}
		return actualTo;
	}
	return nullptr;
}


void windowserver_t::fpsCounter()
{
	platformRegisterTaskIdentifier("windowserver/fps-counter");

	int seconds = 0;
	for(;;)
	{
		if(!debugOn)
		{
			platformSleep(5000);
			continue;
		}

		platformSleep(1000);
		seconds++;
		std::stringstream s;
		s << "FPS: " << framesTotal << ", Time: " << seconds;
		instance()->stateLabel->setTitle(s.str());
		instance()->requestUpdateLater();
		framesTotal = 0;
	}
}

void windowserver_t::setDebug(bool cond)
{
	debugOn = cond;
	server->stateLabel->setVisible(cond);
}

bool windowserver_t::isDebug()
{
	return debugOn;
}
