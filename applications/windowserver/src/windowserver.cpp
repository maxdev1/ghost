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
#include "input/input_receiver.hpp"
#include "interface/registration_thread.hpp"
#include "video/generic_video_output.hpp"
#include "components/window.hpp"
#include "components/text/text_field.hpp"
#include "interface/interface_receiver.hpp"
#include "test.hpp"

#include <cairo/cairo.h>
#include <iostream>
#include <cstdio>
#include <libdevice/interface.hpp>

static windowserver_t* server;
static g_user_mutex dispatchLock = g_mutex_initialize_r(true);
static int framesTotal = 0;
static bool debugOn = false;
int debugBorderCycle = 0;

int main()
{
	server = new windowserver_t();
	server->launch();
	return 0;
}

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
	input_receiver_t::initialize();

	std::string keyLayout = "de-DE";
	if(!g_keyboard::loadLayout(keyLayout))
	{
		klog(("failed to load keyboard layout '" + keyLayout + "', no keyboard input available").c_str());
	}

	server->loadCursor();
	server->requestUpdateLater();
}

void windowserver_t::startOtherTasks()
{
	g_task_register_name("windowserver/launcher");
	// TODO not the windowservers job
	g_spawn("/applications/desktop.bin", "", "", G_SECURITY_LEVEL_APPLICATION);
}

void windowserver_t::startLazyUpdateLoop()
{
	windowserver_t::instance()->updateDebounceLoop();
}

void windowserver_t::launch()
{
	g_task_register_name("windowserver");

	initializeVideo();

	g_create_task((void*) &startInputHandlers);

	g_dimension resolution = videoOutput->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	createVitalComponents(screenBounds);

	test_t::createTestComponents();

	g_create_task((void*) &interfaceRegistrationThread);
	g_create_task((void*) &startOtherTasks);
	g_create_task((void*) &startLazyUpdateLoop);

	renderTask = g_get_tid();
	updateLoop(screenBounds);
}

void windowserver_t::createVitalComponents(g_rectangle screenBounds)
{
	screen = new screen_t();
	screen->setBounds(screenBounds);

	stateLabel = new label_t();
	stateLabel->setTitle("");
	stateLabel->setAlignment(g_text_alignment::RIGHT);
	stateLabel->setVisible(debugOn);
	stateLabel->setBounds(g_rectangle(10, screenBounds.height - 100, screenBounds.width - 20, 30));
	instance()->stateLabel->setColor(RGB(255, 255, 255));
	screen->addChild(stateLabel);

	g_create_task((void*) &windowserver_t::fpsCounter);
}

g_video_output* windowserver_t::findVideoOutput()
{
	klog("waiting for a video device to be present...");

	auto tx = G_MESSAGE_TOPIC_TRANSACTION_START;
	size_t bufLen = 1024;
	uint8_t buf[bufLen];
	while(true)
	{
		auto status = g_receive_topic_message(G_DEVICE_EVENT_TOPIC, buf, bufLen, tx);
		if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto message = (g_message_header*) buf;
			tx = message->transaction;
			auto content = (g_device_event_header*) G_MESSAGE_CONTENT(message);

			if(content->event == G_DEVICE_EVENT_DEVICE_REGISTERED)
			{
				auto deviceEvent = (g_device_event_device_registered*) content;

				if(deviceEvent->type == G_DEVICE_TYPE_VIDEO)
				{
					return new g_generic_video_output(deviceEvent->driver, deviceEvent->id);
				}
			}
		}
	}
}

void windowserver_t::initializeVideo()
{
	videoOutput = findVideoOutput();

	g_set_video_log(false);
	if(!videoOutput->initialize())
	{
		klog("failed to initialize generic video output");
		g_exit(-1);
	}
}

void windowserver_t::updateLoop(const g_rectangle& screenBounds) const
{
	g_task_register_name("windowserver/updater");

	graphics_t global;
	global.resize(screenBounds.width, screenBounds.height, false);

	cursor_t::nextPosition = g_point(screenBounds.width / 2, screenBounds.height / 2);

	uint64_t lastUpdate = 0;
	g_mutex_acquire(updateLock);
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
		g_mutex_acquire_to(updateLock, 1000);

		auto now = g_millis();
		if(now - lastUpdate < 10)
			g_sleep(5);
		lastUpdate = now;
	}
}

void windowserver_t::requestUpdateImmediately() const
{
	g_mutex_release(updateLock);
}

void windowserver_t::requestUpdateLater() const
{
	g_mutex_release(lazyUpdateLock);
}

void windowserver_t::updateDebounceLoop() const
{
	g_mutex_acquire(lazyUpdateLock);
	while(true)
	{
		g_sleep(10);
		g_mutex_acquire_to(lazyUpdateLock, 1000);
		g_mutex_release(updateLock);
	}
}

void windowserver_t::output(graphics_t* graphics) const
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
	videoOutput->blit(invalid, screenBounds, buffer);
}

void windowserver_t::loadCursor()
{
	auto dir = g_open_directory("/system/graphics/cursor");
	g_fs_directory_entry* entry;
	while((entry = g_read_directory(dir)) != nullptr)
	{
		std::string path = std::string("/system/graphics/cursor") + "/" + entry->name;
		cursor_t::load(path);
	}
	cursor_t::set("default");
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

		g_mutex_acquire(dispatchLock);
		handledBy = event.visit(component);
		g_mutex_release(dispatchLock);
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
	g_task_register_name("windowserver/fps-counter");

	int seconds = 0;
	for(;;)
	{
		if(!debugOn)
		{
			g_sleep(5000);
			continue;
		}

		g_sleep(1000);
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
