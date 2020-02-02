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

#include "windowserver.hpp"
#include "output/vbe_video_output.hpp"
#include "input/input_receiver.hpp"
#include "events/event.hpp"
#include "events/locatable.hpp"
#include "interface/registration_thread.hpp"

#include <components/background.hpp>
#include <components/cursor.hpp>
#include <components/window.hpp>
#include <components/checkbox.hpp>
#include <components/text/text_field.hpp>
#include <components/plain_console_panel.hpp>
#include <components/scrollpane.hpp>
#include <components/scrollbar.hpp>
#include <components/button.hpp>
#include <layout/flow_layout_manager.hpp>
#include <layout/grid_layout_manager.hpp>
#include <interface/component_registry.hpp>
#include <ghostuser/ui/properties.hpp>

#include <iostream>
#include <stdio.h>
#include <algorithm>

#include <ghostuser/tasking/lock.hpp>

#include <typeinfo>
#include <cairo/cairo.h>

static windowserver_t* server;
static g_lock dispatch_lock;

#if BENCHMARKING
static uint64_t rounds = 0;
static uint64_t total_event_processing = 0;
static uint64_t total_component_processing = 0;
static uint64_t total_blitting = 0;
#endif

/**
 *
 */
int main(int argc, char** argv) {
	server = new windowserver_t();
	server->launch();
	return 0;
}

/**
 *
 */
void windowserver_t::launch() {
	g_task_register_id("windowserver");
	g_sleep(3000);

	// disable video log
	g_set_video_log(false);

	// initialize the video output
	klog("calling VBE driver");
	video_output = new vbe_video_output_t();
	if (!video_output->initialize()) {
		std::cerr << "failed to initialize video mode" << std::endl;
		klog("failed to initialize video mode");
		return;
	}

	// set up event handling
	event_processor = new event_processor_t();
	input_receiver_t::initialize();

	std::string keyLayout = "de-DE";
	klog(("loading keyboard layout '" + keyLayout + "'").c_str());
	if (!g_keyboard::loadLayout(keyLayout)) {
		klog(("failed to load keyboard layout '" + keyLayout + "', no keyboard input available").c_str());
	}

	// create the cursor
	loadCursor();

	// perform main loop
	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);

	screen = new screen_t();
	screen->setBounds(screenBounds);

	background_t background(screenBounds);
	screen->addChild(&background);

	// start interface
	registration_thread_t* registration_thread = new registration_thread_t();
	registration_thread->start();

	responder_thread = new command_message_responder_thread_t();
	responder_thread->start();

	// test components
	createTestComponents();

// execute the main loop
	mainLoop(screenBounds);
}

static uint64_t render_start;

/**
 *
 */
void lockcheck() {

	while (true) {
		if (g_millis() - render_start > 3000) {
			g_log("window server has frozen");
		}
		g_sleep(1000);
	}
}

/**
 *
 */
void windowserver_t::mainLoop(g_rectangle screenBounds) {

	g_graphics global;
	global.resize(screenBounds.width, screenBounds.height);
	g_create_thread((void*) lockcheck);

	cursor_t::nextPosition = g_point(screenBounds.width / 2, screenBounds.height / 2);

	// intially set rendering atom
	render_atom = true;

	uint64_t render_time;

	while (true) {
		render_start = g_millis();
		event_processor->processMouseState();

#if BENCHMARKING
		rounds++;
#endif

		// do event processing
#if BENCHMARKING
		uint64_t time_event_processing = g_millis();
#endif
		event_processor->process();
#if BENCHMARKING
		total_event_processing += (g_millis() - time_event_processing);
#endif

		// render the screen
#if BENCHMARKING
		uint64_t time_component_processing = g_millis();
#endif
		// make the root component resolve all requirements
		screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT);

		// blit the root component to the buffer
		screen->blit(&global, screenBounds, g_point(0, 0));
#if BENCHMARKING
		total_component_processing += (g_millis() - time_component_processing);
#endif

		// paint the cursor
		cursor_t::paint(&global);

		// blit output
		blit(&global);

		// limit to 60 fps
		render_time = g_millis() - render_start;
		if (render_time < (1000 / 60)) {
			g_sleep((1000 / 60) - render_time);
		}

		g_atomic_lock_to(&render_atom, 100);

		// print output
#if BENCHMARKING
		if (rounds % 100 == 0) {
			klog("benchmark results (1/1000): eventProcessing=%i, componentProcessing=%i, blitting=%i", (total_event_processing * 1000 / rounds),
					(total_component_processing * 1000 / rounds), (total_blitting * 1000 / rounds));
		}
#endif
	}
}

/**
 *
 */
void windowserver_t::blit(g_graphics* graphics) {

	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	g_color_argb* buffer = (g_color_argb*) cairo_image_surface_get_data(graphics->getSurface());

	// get invalid output
	g_rectangle invalid = screen->grabInvalid();

	if (invalid.width == 0 && invalid.height == 0) {
		return;
	}

	// do blitting
#if BENCHMARKING
	uint64_t time_blitting = g_millis();
#endif
	video_output->blit(invalid, screenBounds, buffer);
#if BENCHMARKING
	total_blitting += (g_millis() - time_blitting);
#endif
}

/**
 *
 */
void windowserver_t::loadCursor() {
	cursor_t::load("/system/graphics/cursor/default.cursor");
	cursor_t::load("/system/graphics/cursor/text.cursor");
	cursor_t::load("/system/graphics/cursor/resize-ns.cursor");
	cursor_t::load("/system/graphics/cursor/resize-ew.cursor");
	cursor_t::load("/system/graphics/cursor/resize-nesw.cursor");
	cursor_t::load("/system/graphics/cursor/resize-nwes.cursor");
	cursor_t::set("default");
	cursor_t::focusedComponent = screen;
}

/**
 *
 */
class open_executable_action_handler_t: public internal_action_handler_t {
public:
	std::string exe;
	std::string args;
	open_executable_action_handler_t(std::string exe, std::string args) :
			exe(exe), args(args) {
	}
	void handle(action_component_t* source) {
		g_spawn(exe.c_str(), args.c_str(), "/", G_SECURITY_LEVEL_APPLICATION);
	}
};

/**
 *
 */
void addExecutableButton(window_t* window, std::string name, std::string exe, std::string args) {

	static int nextExeButtonPos = 70;
	button_t* openCalculatorButton = new button_t();
	openCalculatorButton->setBounds(g_rectangle(10, nextExeButtonPos, 270, 30));
	openCalculatorButton->getLabel().setTitle(name);
	openCalculatorButton->setInternalActionHandler(new open_executable_action_handler_t(exe, args));
	window->addChild(openCalculatorButton);
	nextExeButtonPos += 35;
}

/**
 *
 */
void windowserver_t::createTestComponents() {

	window_t* testWindow = new window_t;
	testWindow->setTitle("Welcome to Ghost!");
	testWindow->setBounds(g_rectangle(10, 10, 320, 230));
	testWindow->setNumericProperty(G_UI_PROPERTY_RESIZABLE, false);

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

	/*
	 scrollpane_t* scroller = new scrollpane_t;
	 scroller->setBounds(g_rectangle(0, 0, 300, 200));
	 testWindow->addChild(scroller);

	 panel_t* contentPanel = new panel_t();
	 contentPanel->setBounds(g_rectangle(0, 0, 400, 400));
	 contentPanel->setBackground(RGB(200, 200, 200));
	 contentPanel->setLayoutManager(new grid_layout_manager_t(1, 5));
	 scroller->setViewPort(contentPanel);

	 button_t* button1 = new button_t();
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
	 contentPanel->addChild(label3);
	 */

	screen->addChild(testWindow);
	testWindow->setVisible(true);
}

/**
 *
 */
component_t* windowserver_t::dispatchUpwards(component_t* component, event_t& event) {

	// store when dispatching to parents
	g_point initialPosition;
	locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
	if (locatable) {
		initialPosition = locatable->position;
	}

	// check upwards until someone accepts the event
	component_t* acceptor = component;
	while (!dispatch(acceptor, event)) {
		acceptor = acceptor->getParent();
		if (acceptor == 0) {
			break;
		}

		// restore position on locatable events
		if (locatable) {
			locatable->position = initialPosition;
		}
	}
	return acceptor;
}

/**
 *
 */
bool windowserver_t::dispatch(component_t* component, event_t& event) {

	dispatch_lock.lock();

	bool handled = false;

	if (component->canHandleEvents()) {
		locatable_t* locatable = dynamic_cast<locatable_t*>(&event);
		if (locatable != 0) {
			g_point locationOnScreen = component->getLocationOnScreen();
			locatable->position.x -= locationOnScreen.x;
			locatable->position.y -= locationOnScreen.y;
		}

		handled = component->handle(event);
	}

	dispatch_lock.unlock();
	return handled;
}

/**
 *
 */
windowserver_t* windowserver_t::instance() {
	return server;
}

/**
 *
 */
void windowserver_t::triggerRender() {
	render_atom = false;
}


