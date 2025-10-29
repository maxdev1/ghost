#ifdef _GHOST_

#include "platform/platform.hpp"
#include "platform/ghost/ghost.hpp"
#include "windowserver.hpp"
#include "components/cursor.hpp"
#include "events/event_processor.hpp"

#include <libinput/keyboard/keyboard.hpp>
#include <libinput/mouse/mouse.hpp>
#include <libps2driver/ps2driver.hpp>

static g_fd keyboardIn;
static g_fd mouseIn;

void inputReceiverInitialize()
{
	g_tid keyEventThread = g_create_task((void*) inputReceiverStartReceiveKeyEvents);
	g_tid mouseEventThread = g_create_task((void*) inputReceiverStartReceiveMouseEvents);
	ps2DriverInitialize(&keyboardIn, &mouseIn, keyEventThread, mouseEventThread);
}

void inputReceiverStartReceiveKeyEvents()
{
	platformRegisterTaskIdentifier("windowserver/key-receiver");

	event_processor_t* event_queue = windowserver_t::instance()->eventProcessor;

	while(true)
	{
		g_key_info key = g_keyboard::readKey(keyboardIn);

		if(key.ctrl && key.key == "KEY_Q" && key.pressed)
		{
			windowserver_t::setDebug(!windowserver_t::isDebug());
			continue;
		}

		event_queue->bufferKeyEvent(key);

		windowserver_t::instance()->requestUpdateImmediately();
	}
}

void inputReceiverStartReceiveMouseEvents()
{
	platformRegisterTaskIdentifier("windowserver/mouse-receiver");

	windowserver_t* instance = windowserver_t::instance();
	g_dimension resolution = instance->videoOutput->getResolution();

	while(true)
	{
		g_mouse_info info = g_mouse::readMouse(mouseIn);

		cursor_t::nextPosition.x += info.x;
		cursor_t::nextPosition.y += info.y;

		if(cursor_t::nextPosition.x < 0)
		{
			cursor_t::nextPosition.x = 0;
		}
		if(cursor_t::nextPosition.x > resolution.width - 2)
		{
			cursor_t::nextPosition.x = resolution.width - 2;
		}
		if(cursor_t::nextPosition.y < 0)
		{
			cursor_t::nextPosition.y = 0;
		}
		if(cursor_t::nextPosition.y > resolution.height - 2)
		{
			cursor_t::nextPosition.y = resolution.height - 2;
		}

		cursor_t::nextPressedButtons = G_MOUSE_BUTTON_NONE;
		if(info.button1)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_1;
		}
		if(info.button2)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_2;
		}
		if(info.button3)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_3;
		}

		cursor_t::nextScroll += info.scroll;

		windowserver_t::instance()->requestUpdateImmediately();
	}
}

#endif

