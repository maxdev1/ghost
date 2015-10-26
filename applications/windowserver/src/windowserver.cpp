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

#include <components/background.hpp>
#include <components/cursor.hpp>
#include <components/window.hpp>
#include <components/checkbox.hpp>
#include <components/text/text_field.hpp>
#include <components/plain_console_panel.hpp>
#include <components/button.hpp>
#include <layout/flow_layout_manager.hpp>
#include <layout/grid_layout_manager.hpp>

#include <iostream>
#include <stdio.h>

#include <ghostuser/tasking/lock.hpp>

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

	// initialize the video output
	video_output = new vbe_video_output_t();
	if (!video_output->initialize()) {
		std::cerr << "failed to initialize video mode" << std::endl;
		klog("failed to initialize video mode");
	}

	// set up event handling
	event_processor = new event_processor_t();
	input_receiver_t::initialize();

	std::string keyLayout = "de-DE";
	klog(("loading keyboard layout '" + keyLayout + "'").c_str());
	if (!g_keyboard::loadLayout(keyLayout)) {
		klog(("failed to load keyboard layout '" + keyLayout + "', no keyboard input available").c_str());
	}

	// perform main loop
	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);

	screen = new screen_t();
	screen->setBounds(screenBounds);

	background_t background(screenBounds);
	screen->addChild(&background);

	// Create the cursor
	cursor_t::load("/system/graphics/cursor/default.cursor");
	cursor_t::load("/system/graphics/cursor/text.cursor");
	cursor_t::set("default");
	cursor_t::focusedComponent = screen;

	// create test components
	createTestComponents();

	g_graphics global;
	g_painter globalPainter(global);
	global.resize(screenBounds.width, screenBounds.height);

	while (true) {

#if BENCHMARKING
		rounds++;
#endif
		// set the lock so it can be unset later
		execution_state.lock = true;

		// do event processing
#if BENCHMARKING
		uint64_t time_event_processing = g_millis();
#endif
		event_processor->process();
#if BENCHMARKING
		total_event_processing += (g_millis() - time_event_processing);
#endif

		// render the screen
		render(&global, &globalPainter);

		// block until rendering is requested
		g_atomic_lock(&execution_state.lock);

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
void windowserver_t::render(g_graphics* graphics, g_painter* painter) {

	g_color_argb* buffer = graphics->getBuffer();

	// do component processing
#if BENCHMARKING
	uint64_t time_component_processing = g_millis();
#endif
	// make the root component resolve all requirements
	screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE);
	screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT);
	screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT);

	// blit the root component to the buffer
	g_dimension resolution = video_output->getResolution();
	g_rectangle screenBounds(0, 0, resolution.width, resolution.height);
	screen->blit(buffer, screenBounds, screenBounds, g_point(0, 0));
#if BENCHMARKING
	total_component_processing += (g_millis() - time_component_processing);
#endif

	// paint the cursor
	cursor_t::paint(painter);

	g_rectangle invalid = screen->grabInvalid();

	// quit if nothing to blit
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
void windowserver_t::createTestComponents() {

	// Window: layouted
	window_t* layoutedWindow = new window_t;
	layoutedWindow->setBounds(g_rectangle(10, 10, 300, 250));
	layoutedWindow->setLayoutManager(new flow_layout_manager_t());
	screen->addChild(layoutedWindow);

	button_t* button1 = new button_t();
	button1->getLabel().setTitle("Button 1");
	layoutedWindow->addChild(button1);

	button_t* button2 = new button_t();
	button2->getLabel().setTitle("Button 2");
	layoutedWindow->addChild(button2);

	label_t* label1 = new label_t();
	label1->setTitle("I am a label");
	layoutedWindow->addChild(label1);

	label_t* label2 = new label_t();
	label2->setTitle("These labels float!");
	layoutedWindow->addChild(label2);

	label_t* label3 = new label_t();
	label3->setTitle("This is an example text.");
	layoutedWindow->addChild(label3);

	checkbox_t* testCheckBox = new checkbox_t;
	testCheckBox->setBounds(g_rectangle(10, 200, 100, 20));
	testCheckBox->getLabel().setTitle("I'm a checkbox!");
	layoutedWindow->addChild(testCheckBox);

	layoutedWindow->setVisible(true);

	// Window: console
	window_t* consolePanelTestWindow = new window_t;
	consolePanelTestWindow->setBounds(g_rectangle(330, 10, 300, 300));
	consolePanelTestWindow->setLayoutManager(new grid_layout_manager_t(1, 1));

	plain_console_panel_t* console = new plain_console_panel_t;
	consolePanelTestWindow->addChild(console);

	std::string test = "This is a test text.\nI basically want to test how the console renders this. :)\n\nAnd it looooks....";
	for (int i = 0; i < test.length(); i++) {
		console->append(test[i]);
	}

	screen->addChild(consolePanelTestWindow);

	consolePanelTestWindow->setVisible(true);

	// Window: GhostInfo
	window_t* ghostInfoWindow = new window_t;
	ghostInfoWindow->setBounds(g_rectangle(60, 140, 320, 200));

	label_t* testLabel = new label_t;
	testLabel->setBounds(g_rectangle(10, 10, 480, 40));
	testLabel->setTitle("Welcome to Ghost!\nThis is a small UI demonstration.\nWritten by Max Schlüssel");
	ghostInfoWindow->addChild(testLabel);

	text_field_t* testField = new text_field_t;
	testField->setBounds(g_rectangle(10, 60, 300, 30));
	ghostInfoWindow->addChild(testField);

	text_field_t* testField2 = new text_field_t;
	testField2->setBounds(g_rectangle(10, 100, 300, 70));
	ghostInfoWindow->addChild(testField2);

	screen->addChild(ghostInfoWindow);
	ghostInfoWindow->bringToFront();

	ghostInfoWindow->setVisible(true);
}
/**
 *
 */
component_t* windowserver_t::dispatchUpwards(component_t* component, event_t& event) {

	component_t* acceptor = component;
	while (!dispatch(acceptor, event)) {
		acceptor = acceptor->getParent();
		if (acceptor == 0) {
			break;
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
void windowserver_t::request_step() {
	execution_state.lock = false;
}

/**
 *
 */
windowserver_t* windowserver_t::instance() {
	return server;
}
