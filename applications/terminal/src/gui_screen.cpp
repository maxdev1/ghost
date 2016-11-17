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

#include <gui_screen.hpp>
#include <string.h>

#include <ghost.h>
#include <list>
#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/tasking/lock.hpp>

uint8_t paint_uptodate = false;
bool cursorBlink = false;
bool focused = false;
uint64_t lastInput = 0;

std::list<g_key_info> waitingInput;
uint8_t noInputAvailable = true;
g_lock waitingInputLock;

std::string output;
gui_screen_t* instance;

class input_key_listener_t: public g_key_listener {

	virtual void handle_key_event(g_key_event& e) {
		waitingInputLock.lock();
		waitingInput.push_back(g_keyboard::fullKeyInfo(e.info));
		lastInput = g_millis();
		noInputAvailable = false;
		waitingInputLock.unlock();
	}
};

class canvas_resize_bounds_listener_t: public g_bounds_listener {
public:
	g_canvas* canvas;

	canvas_resize_bounds_listener_t(g_canvas* canvas) :
			canvas(canvas) {
	}

	virtual void handle_bounds_changed(g_rectangle bounds) {
		// TODO handle this through a layout manager

		// TODO why does this kill the server?
		// canvas->setBounds(g_rectangle(0, 0, bounds.width, bounds.height));
		paint_uptodate = false;
	}
};

class canvas_buffer_listener_t: public g_canvas_buffer_listener {
public:
	virtual void handle_buffer_changed() {
		paint_uptodate = false;
	}
};

class terminal_focus_listener_t: public g_focus_listener {
public:
	virtual void handle_focus_changed(bool now_focused) {
		focused = now_focused;
		paint_uptodate = false;
		lastInput = g_millis();
	}
};

/**
 *
 */
gui_screen_t::gui_screen_t() {
	instance = this;
	initialize();
}

/**
 *
 */
void gui_screen_t::initialize() {

	window = g_window::create();
	window->setTitle("Terminal");
	window->onClose([] {
		klog("exiting terminal");
		g_exit(0);
	});

	canvas = g_canvas::create();
	window->addChild(canvas);

	window->setBoundsListener(new canvas_resize_bounds_listener_t(canvas));

	g_rectangle windowBounds = g_rectangle(200, 200, 400, 250);
	window->setBounds(windowBounds);
	window->setVisible(true);
	canvas->setBounds(
			g_rectangle(0, 0, windowBounds.width, windowBounds.height));

	canvas->setListener(G_UI_COMPONENT_EVENT_TYPE_KEY,
			new input_key_listener_t());
	canvas->setBufferListener(new canvas_buffer_listener_t());
	window->setListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS,
			new terminal_focus_listener_t());

	font = g_font_loader::get("consolas");
	viewModel = g_text_layouter::getInstance()->initializeBuffer();

	g_create_thread((void*) &paint_entry);
}

/**
 *
 */
void gui_screen_t::paint_entry() {
	instance->paint();
}

void gui_screen_t::blinkCursorThread() {
	while (true) {
		cursorBlink = !cursorBlink;
		paint_uptodate = false;
		g_sleep(650);
	}
}

/**
 *
 */
void gui_screen_t::paint() {

	g_create_thread((void*) blinkCursorThread);

	int padding = 5;
	while (true) {
		auto windowBounds = window->getBounds();
		canvas->setBounds(
				g_rectangle(0, 0, windowBounds.width, windowBounds.height));

		auto cr = getGraphics();
		if (cr == 0) {
			g_sleep(100);
			continue;
		}

		// clear
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0, 0, 0, 1);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_restore(cr);

		// relayout text
		g_text_layouter::getInstance()->layout(cr, output.c_str(), font, 14,
				g_rectangle(padding, padding,
						windowBounds.width - 2 * padding - 20,
						windowBounds.height - 2 * padding),
				g_text_alignment::LEFT, viewModel, true);

		// check which is the lowest-down-the-screen character
		int highesty = 0;
		for (g_positioned_glyph& g : viewModel->positions) {
			int ypos = g.position.y - g.glyph->y;
			if (ypos > highesty) {
				highesty = ypos;
			}
		}

		// calculate limit
		int yLimit = windowBounds.height - 60;
		int yOffset = 0;
		if (highesty > yLimit) {
			yOffset = yLimit - highesty;
		}

		// paint characters
		g_point last;
		cairo_set_source_rgba(cr, 1, 1, 1, 1);
		for (g_positioned_glyph& g : viewModel->positions) {
			last = g.position;

			cairo_save(cr);
			cairo_translate(cr, g.position.x - g.glyph->x,
					yOffset + g.position.y - g.glyph->y);
			cairo_glyph_path(cr, g.glyph, g.glyph_count);
			cairo_fill(cr);
			cairo_restore(cr);
		}

		// paint cursor
		if (focused) {
			if ((g_millis() - lastInput < 300) || cursorBlink) {
				cairo_save(cr);
				cairo_set_source_rgba(cr, 1, 1, 1, 1);
				cairo_rectangle(cr, last.x + 10, yOffset + last.y - 12, 8, 14);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}

		canvas->blit(g_rectangle(0, 0, bufferSize.width, bufferSize.height));

		paint_uptodate = true;
		g_atomic_block(&paint_uptodate);
	}
}

/**
 *
 */
cairo_t* gui_screen_t::getGraphics() {

	// get buffer
	auto bufferInfo = canvas->getBuffer();
	if (bufferInfo.buffer == 0) {
		return 0;
	}

	bufferSize.width = bufferInfo.width;
	bufferSize.height = bufferInfo.height;

	// get the surface ready and go:
	if (existingSurface == 0 || existingSurfaceBuffer != bufferInfo.buffer) {
		if (existingContext != 0) {
			cairo_destroy(existingContext);
		}

		existingSurface = cairo_image_surface_create_for_data(
				(uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32,
				bufferInfo.width, bufferInfo.height,
				cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
						bufferInfo.width));
		existingSurfaceBuffer = bufferInfo.buffer;
		existingContext = cairo_create(existingSurface);
	}

	return existingContext;
}

/**
 *
 */
bool charIsUtf8(char c) {
	if ((c == 0x09 || c == 0x0A || c == 0x0D || (0x20 <= c && c <= 0x7E))) {
		return true;
	}
	return false;
}

/**
 *
 */
void gui_screen_t::clean() {
	output = "";
	paint_uptodate = false;
}

/**
 *
 */
void gui_screen_t::activate() {
}

/**
 *
 */
void gui_screen_t::deactivate() {
}

/**
 *
 */
void gui_screen_t::updateCursor() {
}

/**
 *
 */
void gui_screen_t::writeChar(char c) {
	if (charIsUtf8(c)) {
		output += c;
		paint_uptodate = false;
	}
}

/**
 *
 */
void gui_screen_t::backspace() {
	output = output.substr(0, output.length() - 1);
	paint_uptodate = false;
}

/**
 *
 */
g_key_info gui_screen_t::readInput(bool* cancelCondition) {

	if (waitingInput.size() == 0) {
		if (cancelCondition) {
			g_atomic_block_dual(&noInputAvailable, (uint8_t*) &cancelCondition);
		} else {
			g_atomic_block(&noInputAvailable);
		}
	}

	waitingInputLock.lock();

	g_key_info result = waitingInput.front();
	waitingInput.pop_front();
	if (waitingInput.size() == 0) {
		noInputAvailable = true;
	}

	waitingInputLock.unlock();

	return result;
}

/**
 *
 */
void gui_screen_t::workingDirectoryChanged(std::string str) {
	window->setTitle("Terminal - " + str);
}

/**
 * TODO
 */
void gui_screen_t::moveCursor(int x, int y) {
}
int gui_screen_t::getCursorX() {
	return 0;
}
int gui_screen_t::getCursorY() {
	return 0;
}
