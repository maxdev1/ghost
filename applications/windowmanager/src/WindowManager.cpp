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

#include <components/Button.hpp>

#include <ghostuser/graphics/Graphics.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <components/Background.hpp>
#include <components/Screen.hpp>
#include <components/Panel.hpp>
#include <components/PlainConsolePanel.hpp>
#include <components/text/TextField.hpp>
#include <components/Label.hpp>
#include <components/Window.hpp>
#include <components/CheckBox.hpp>
#include <events/MouseEvent.hpp>
#include <events/KeyEvent.hpp>
#include <events/FocusEvent.hpp>
#include <events/Locatable.hpp>
#include <input/InputManager.hpp>
#include <interface/RequestHandler.hpp>
#include <WindowManager.hpp>
#include <FPSCounterThread.hpp>
#include <Cursor.hpp>

#include <layout/FlowLayoutManager.hpp>
#include <layout/GridLayoutManager.hpp>
#include <ghostuser/tasking/Lock.hpp>
#include <ghost.h>

#include <string.h>

static WindowManager instance;
static g_lock dispatchLock;

static Label* fpsLabel = 0;

static g_lock keyEventQueueLock;
static g_lock mouseEventQueueLock;

/**
 * Runs the instance of the window manager singleton
 */
int main() {
	instance.run();
	return 0;
}

/**
 * Returns the window manager instance
 *
 * @return the instance
 */
WindowManager* WindowManager::getInstance() {
	return &instance;
}

/**
 *
 */
void WindowManager::run() {
	g_task_register_id("windowserver");
	g_logger::log("initialized");

	multiclickTimespan = DEFAULT_MULTICLICK_TIMESPAN;

	int width = 800;
	int height = 600;
	int bits = 24;

	g_logger::log("setting video mode to %ix%ix%i", width, height, bits);
	if (g_vbe::setMode(width, height, bits, graphicsInfo)) { // successful

		std::string currentLayout = "de-DE";
		g_logger::log("loading keyboard layout '" + currentLayout + "'");
		if (g_keyboard::loadLayout(currentLayout)) {

			systemLoop();
		} else {
			g_logger::log("failed to load keyboard layout '" + currentLayout + "'");
		}

	} else {
		g_logger::log("vesa driver reported that mode-setting was not successful, quitting");
	}

}

/**
 *
 */
void runRequestHandler() {
	RequestHandler req;
	req.run();
}

/**
 *
 */
void WindowManager::systemLoop() {

	// register as main
	g_task_register_id("windowserver:main");

	// Initialize the input manager
	InputManager::initialize();

	// Create the screen
	g_rectangle screenBounds(0, 0, graphicsInfo.resX, graphicsInfo.resY);
	screen = new screen_t();
	screen->setBounds(screenBounds);

	// Create the background
	Background background(screenBounds);
	screen->addChild(&background);

	// Run FPS counter
#if SHOW_FPS
	g_create_thread((void*) FPSCounterThread::run);
#endif

	// Run request handler
	g_create_thread((void*) runRequestHandler);

	// Create the cursor
	Cursor::load("/system/graphics/cursor/default.cursor");
	Cursor::load("/system/graphics/cursor/text.cursor");
	Cursor::set("default");
	Cursor::focusedComponent = screen;

#if SHOW_FPS
	// FPS label
	fpsLabel = new Label();
	fpsLabel->setBounds(g_rectangle(0, 0, 100, 20));
	screen->addChild(fpsLabel);
#endif

	// TODO remove test components
	// createTestComponents();

	// system loop
	g_graphics global;
	g_painter globalPainter(global);
	global.resize(screenBounds.width, screenBounds.height);
	while (true) {

		// Set render atom
		renderAtom = true;

		// Handle waiting events
		keyEventQueueLock.lock();
		while (keyEventQueue.size() > 0) {
			translateKeyEvent(keyEventQueue.front());
			keyEventQueue.pop_front();
		}
		keyEventQueueLock.unlock();

		mouseEventQueueLock.lock();
		while (mouseEventQueue.size() > 0) {
			translateMouseEvent(mouseEventQueue.front());
			mouseEventQueue.pop_front();
		}
		mouseEventQueueLock.unlock();

		// Requirements
		screen->resolveRequirement(COMPONENT_REQUIREMENT_UPDATE);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_LAYOUT);
		screen->resolveRequirement(COMPONENT_REQUIREMENT_PAINT);
		// Blit
		screen->blit(global.getBuffer(), screenBounds, screenBounds, g_point(0, 0));
		// Add cursor
		Cursor::paint(&globalPainter);

#if SHOW_FPS
		// Show FPS
		FPSCounterThread::tick();
#endif

		// Output
		outputDirty(screen->grabInvalid(), screenBounds, global.getBuffer());

		// Wait until another render is requested
		g_atomic_lock(&renderAtom);
	}
}

/**
 *
 */
void WindowManager::addToScreen(Component* comp) {
	screen->addChild(comp);
}

/**
 *
 */
void WindowManager::markForRender() {
	renderAtom = false;
}

/**
 *
 */
void WindowManager::outputDirty(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source) {

	uint8_t* out = (uint8_t*) graphicsInfo.lfb;

	for (int y = invalid.y; y < invalid.y + invalid.height; y++) {
		for (int x = invalid.x; x < invalid.x + invalid.width; x++) {

			g_color_argb color = source[y * sourceSize.width + x];

			// TODO convert for each output format

			uint32_t off = y * graphicsInfo.bpsl + x * 3;
			out[off] = color & 0xFF;
			out[off + 1] = (color >> 8) & 0xFF;
			out[off + 2] = (color >> 16) & 0xFF;
		}
	}
}

/**
 *
 */
Component* WindowManager::dispatchUpwards(Component* component, Event& event) {

	Component* acceptor = component;
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
bool WindowManager::dispatch(Component* component, Event& event) {

	dispatchLock.lock();

	bool handled = false;

	if (component->canHandleEvents()) {
		Locatable* locatable = dynamic_cast<Locatable*>(&event);
		if (locatable != 0) {
			g_point locationOnScreen = component->getLocationOnScreen();
			locatable->position.x -= locationOnScreen.x;
			locatable->position.y -= locationOnScreen.y;
		}

		handled = component->handle(event);
	}

	dispatchLock.unlock();
	return handled;
}

/**
 *
 */
void WindowManager::queueKeyEvent(g_key_info& info) {

	keyEventQueueLock.lock();
	keyEventQueue.push_back(info);
	keyEventQueueLock.unlock();

	// Mark for render
	markForRender();
}

/**
 *
 */
void WindowManager::queueMouseEvent(g_mouse_info& info) {

	mouseEventQueueLock.lock();
	mouseEventQueue.push_back(info);
	mouseEventQueueLock.unlock();

	// Mark for render
	markForRender();
}

/**
 *
 */
void WindowManager::translateKeyEvent(g_key_info& info) {

	if (Cursor::focusedComponent) {
		KeyEvent k;
		k.info = info;
		dispatch(Cursor::focusedComponent, k);
	}
}

/**
 *
 */
void WindowManager::translateMouseEvent(g_mouse_info& info) {

	g_point previousPosition = Cursor::position;
	MouseButton previousPressedButtons = Cursor::pressedButtons;

	// Invalidate old location
	screen->markDirty(Cursor::getArea());

	// Calculate new cursor position
	Cursor::position.x += info.x;
	Cursor::position.y -= info.y;

	if (Cursor::position.x < 0) {
		Cursor::position.x = 0;
	}
	if (Cursor::position.x > graphicsInfo.resX - 2) {
		Cursor::position.x = graphicsInfo.resX - 2;
	}
	if (Cursor::position.y < 0) {
		Cursor::position.y = 0;
	}
	if (Cursor::position.y > graphicsInfo.resY - 2) {
		Cursor::position.y = graphicsInfo.resY - 2;
	}

	// Invalidate new location
	screen->markDirty(Cursor::getArea());

	// Set pressed buttons
	Cursor::pressedButtons = MOUSE_BUTTON_NONE;
	if (info.button1) {
		Cursor::pressedButtons |= MOUSE_BUTTON_1;
	}
	if (info.button2) {
		Cursor::pressedButtons |= MOUSE_BUTTON_2;
	}
	if (info.button3) {
		Cursor::pressedButtons |= MOUSE_BUTTON_3;
	}

	MouseEvent baseEvent;
	baseEvent.screenPosition = Cursor::position;
	baseEvent.position = baseEvent.screenPosition;
	baseEvent.buttons = Cursor::pressedButtons;

	// Press
	if ((!(previousPressedButtons & MOUSE_BUTTON_1) && (Cursor::pressedButtons & MOUSE_BUTTON_1))
			|| (!(previousPressedButtons & MOUSE_BUTTON_2) && (Cursor::pressedButtons & MOUSE_BUTTON_2))
			|| (!(previousPressedButtons & MOUSE_BUTTON_3) && (Cursor::pressedButtons & MOUSE_BUTTON_3))) {

		// Prepare event
		MouseEvent pressEvent = baseEvent;
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
		dispatch(screen, pressEvent);

		Component* hitComponent = screen->getComponentAt(Cursor::position);
		if (hitComponent != 0) {

			// Prepare drag
			if (hitComponent != screen) {
				Cursor::draggedComponent = hitComponent;
			}

			// Switch focus
			if (hitComponent != Cursor::focusedComponent) {
				// Old loses focus
				if (Cursor::focusedComponent != 0) {
					FocusEvent focusLostEvent;
					focusLostEvent.type = FOCUS_EVENT_LOST;
					dispatchUpwards(Cursor::focusedComponent, focusLostEvent);
				}

				// Bring hit components window to front
				Window* parentWindow = hitComponent->getWindow();
				if (parentWindow != 0) {
					parentWindow->bringToFront();
				}

				// New gains focus
				FocusEvent focusGainedEvent;
				focusGainedEvent.type = FOCUS_EVENT_GAINED;
				Cursor::focusedComponent = dispatchUpwards(hitComponent, focusGainedEvent);
			}
		}

		// Release
	} else if (((previousPressedButtons & MOUSE_BUTTON_1) && !(Cursor::pressedButtons & MOUSE_BUTTON_1))
			|| ((previousPressedButtons & MOUSE_BUTTON_2) && !(Cursor::pressedButtons & MOUSE_BUTTON_2))
			|| ((previousPressedButtons & MOUSE_BUTTON_3) && !(Cursor::pressedButtons & MOUSE_BUTTON_3))) {

		if (Cursor::draggedComponent) {
			MouseEvent releaseDraggedEvent = baseEvent;
			releaseDraggedEvent.type = MOUSE_EVENT_DRAG_RELEASE;
			dispatchUpwards(Cursor::draggedComponent, releaseDraggedEvent);
			Cursor::draggedComponent = 0;
		}

		MouseEvent releaseEvent = baseEvent;
		releaseEvent.type = MOUSE_EVENT_RELEASE;
		dispatch(screen, releaseEvent);

		// Move or drag
	} else if (Cursor::position != previousPosition) {

		Component* hovered = screen->getComponentAt(Cursor::position);
		if (hovered != 0 && (hovered != Cursor::hoveredComponent)) {

			// Leave
			if (Cursor::hoveredComponent) {
				MouseEvent leaveEvent = baseEvent;
				leaveEvent.type = MOUSE_EVENT_LEAVE;
				dispatchUpwards(Cursor::hoveredComponent, leaveEvent);
				Cursor::hoveredComponent = 0;
			}

			// Enter
			MouseEvent enterEvent = baseEvent;
			enterEvent.type = MOUSE_EVENT_ENTER;
			Cursor::hoveredComponent = hovered;
			dispatchUpwards(Cursor::hoveredComponent, enterEvent);
		}

		if (Cursor::draggedComponent != 0) { // Dragging
			MouseEvent dragEvent = baseEvent;
			dragEvent.type = MOUSE_EVENT_DRAG;
			dispatchUpwards(Cursor::draggedComponent, dragEvent);

		} else { // Moving
			MouseEvent moveEvent = baseEvent;
			moveEvent.type = MOUSE_EVENT_MOVE;
			dispatch(screen, moveEvent);
		}
	}
}

/**
 *
 */
Label* WindowManager::getFpsLabel() {
	return fpsLabel;
}

/**
 *
 */
void WindowManager::createTestComponents() {

	// Window: layouted
	Window* layoutedWindow = new Window;
	layoutedWindow->setBounds(g_rectangle(10, 10, 300, 250));
	layoutedWindow->setLayoutManager(new FlowLayoutManager());
	screen->addChild(layoutedWindow);

	Button* button1 = new Button();
	button1->getLabel().setTitle("Button 1");
	layoutedWindow->addChild(button1);

	Button* button2 = new Button();
	button2->getLabel().setTitle("Button 2");
	layoutedWindow->addChild(button2);

	Label* label1 = new Label();
	label1->setTitle("I am a label");
	layoutedWindow->addChild(label1);

	Label* label2 = new Label();
	label2->setTitle("These labels float!");
	layoutedWindow->addChild(label2);

	Label* label3 = new Label();
	label3->setTitle("This is an example text.");
	layoutedWindow->addChild(label3);

	CheckBox* testCheckBox = new CheckBox;
	testCheckBox->setBounds(g_rectangle(10, 200, 100, 20));
	testCheckBox->getLabel().setTitle("I'm a checkbox!");
	layoutedWindow->addChild(testCheckBox);

	layoutedWindow->setVisible(true);

	// Window: console
	Window* consolePanelTestWindow = new Window;
	consolePanelTestWindow->setBounds(g_rectangle(330, 10, 300, 300));
	consolePanelTestWindow->setLayoutManager(new GridLayoutManager(1, 1));

	PlainConsolePanel* console = new PlainConsolePanel;
	consolePanelTestWindow->addChild(console);

	std::string test = "This is a test text.\nI basically want to test how the console renders this. :)\n\nAnd it looooks....";
	for (int i = 0; i < test.length(); i++) {
		console->append(test[i]);
	}

	screen->addChild(consolePanelTestWindow);

	consolePanelTestWindow->setVisible(true);

	// Window: GhostInfo
	Window* ghostInfoWindow = new Window;
	ghostInfoWindow->setBounds(g_rectangle(60, 140, 320, 200));

	Label* testLabel = new Label;
	testLabel->setBounds(g_rectangle(10, 10, 480, 40));
	testLabel->setTitle("Welcome to Ghost!\nThis is a small UI demonstration.\nWritten by Max Schlüssel");
	ghostInfoWindow->addChild(testLabel);

	TextField* testField = new TextField;
	testField->setBounds(g_rectangle(10, 60, 300, 30));
	ghostInfoWindow->addChild(testField);

	TextField* testField2 = new TextField;
	testField2->setBounds(g_rectangle(10, 100, 300, 70));
	ghostInfoWindow->addChild(testField2);

	screen->addChild(ghostInfoWindow);
	ghostInfoWindow->bringToFront();

	ghostInfoWindow->setVisible(true);
}
