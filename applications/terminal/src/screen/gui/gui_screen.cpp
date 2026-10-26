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

#include "gui_screen.hpp"
#include <ghost.h>
#include <libinput/keyboard/keyboard.hpp>
#include <list>
#include <cstdio>
#include <cstring>
#include <vector>
#include <climits>
#include <libfenster/color_argb.hpp>
#include <libfenster/layout/grid_layout.hpp>

void guiScreenBlinkCursorEntry(gui_screen_t* screen);
void guiScreenPaintEntry(gui_screen_t* screen);

bool gui_screen_t::initialize(g_user_mutex exitFlag)
{
	this->exitFlag = exitFlag;
	g_mutex_acquire(upToDate); // Arm the repaint latch before UI events arrive.
	inputBufferLock = g_mutex_initialize();
	inputBufferEmpty = g_mutex_initialize();
	g_mutex_acquire(inputBufferEmpty);

	if(!createUi())
		return false;

	g_create_task_d((void*) guiScreenPaintEntry, this);
	g_create_task_d((void*) guiScreenBlinkCursorEntry, this);
	return true;
}

void guiScreenBlinkCursorEntry(gui_screen_t* screen)
{
	while(true)
	{
		screen->blinkCursor();
		g_sleep(500);
	}
}

void guiScreenPaintEntry(gui_screen_t* screen)
{
	screen->paint();
}

bool gui_screen_t::createUi()
{
	auto status = Application::open();
	if(status != fenster::ApplicationOpenStatus::Success)
	{
		klog("terminal: failed to initialize application UI with status %i", status);
		return false;
	}

	window = Window::create();
	window->onClose([this]()
	{
		g_mutex_release(exitFlag);
	});
	window->setTitle("Terminal");
	window->setBackground(ARGB(250, 0, 0, 2));

	canvas.component = Canvas::create();
	canvas.component->setBufferListener(new canvas_buffer_listener_t(this));
	canvas.component->setBoundsListener(new canvas_resize_bounds_listener_t(this));
	canvas.component->addKeyListener([this](KeyEvent& e)
	{
		g_key_info_basic ghostKey;
		ghostKey.pressed = e.info.pressed;
		ghostKey.ctrl = e.info.ctrl;
		ghostKey.shift = e.info.shift;
		ghostKey.alt = e.info.alt;
		ghostKey.scancode = e.info.scancode;

		auto info = g_keyboard::fullKeyInfo(ghostKey);
		if(info.key == "KEY_PAD_2")
		{
			scroll(1);
		}
		else if(info.key == "KEY_PAD_8")
		{
			scroll(-1);
		}
		else
		{
			bufferInput(info);
		}
		g_mutex_acquire(stateLock);
		cursor.blink = true;
		g_mutex_release(stateLock);
	});
	canvas.component->addMouseListener([this](ComponentMouseEvent* event)
	{
		if(event->type == fenster::MouseEventType::Scroll)
		{
			scroll(-event->scroll);
		}
		return this;
	});

	GridLayout::create(window);
	window->addChild(canvas.component);

	Rectangle windowBounds = Rectangle(80, 80, 700, 500);
	window->setBounds(windowBounds);
	window->setVisible(true);

	window->addListener(fenster::ComponentEventType::Focus, new terminal_focus_listener_t(this));
	window->setFocused(true);

	return true;
}

void gui_screen_t::blinkCursor()
{
	g_mutex_acquire(stateLock);
	cursor.blink = !cursor.blink;
	repaint();
	g_mutex_release(stateLock);
}

void canvas_resize_bounds_listener_t::handleBoundsChanged(Rectangle bounds)
{
	screen->setCanvasBounds(bounds);
	screen->repaint();
}

g_key_info gui_screen_t::readInput()
{
	g_key_info result;

	for(;;)
	{
		g_mutex_acquire(inputBufferLock);
		if(inputBuffer.size() == 0)
		{
			g_mutex_release(inputBufferLock);
			g_mutex_acquire(inputBufferEmpty);
		}
		else
		{
			result = inputBuffer.front();
			inputBuffer.pop_front();
			g_mutex_release(inputBufferLock);
			break;
		}
	}

	return result;
}

void gui_screen_t::bufferInput(const g_key_info& info)
{
	g_mutex_acquire(inputBufferLock);
	inputBuffer.push_back(info);
	g_mutex_acquire(stateLock);
	lastInputTime = g_millis();
	g_mutex_release(stateLock);
	g_mutex_release(inputBufferEmpty);
	g_mutex_release(inputBufferLock);
	g_yield();
}

void canvas_buffer_listener_t::handleBufferChanged()
{
	g_mutex_acquire(screen->stateLock);
	screen->fullRepaint = true;
	g_mutex_release(screen->stateLock);
	screen->repaint();
}

void terminal_focus_listener_t::handleFocusChanged(bool now_focused)
{
	screen->setFocused(now_focused);
	screen->repaint();
}

void gui_screen_t::paint()
{
	std::vector<char> previous, current;
	int previousColumns = 0, previousRows = 0;
	int lastCursorX = -1, lastCursorY = -1;
	uint64_t nextPaint = 0;
	repaint();
	while(true)
	{
		g_mutex_acquire(upToDate);
		const auto now = g_millis();
		if(now < nextPaint)
			g_sleep(nextPaint - now);
		nextPaint = g_millis() + 16;
		__atomic_store_n(&repaintPending, false, __ATOMIC_RELEASE);
		auto cr = canvas.component->acquireGraphics();
		if(!cr)
			continue; // Buffer installation will request a repaint.

		g_mutex_acquire(stateLock);
		const int columns = getColumns(), rows = getRows();
		if(columns <= 0 || rows <= 0)
		{
			g_mutex_release(stateLock);
			canvas.component->releaseGraphics();
			continue;
		}
		const bool full = fullRepaint || columns != previousColumns || rows != previousRows;
		fullRepaint = false;
		const int startRow = documentRows < rows ? documentRows : rows + scrollY;
		current.resize((size_t) columns * rows);
		document->copyRows(startRow - 1, columns, rows, current.data());
		const int cursorX = cursor.x > 0 ? (cursor.x - 1) % columns + 1 : 0;
		const int cursorY = startRow + cursor.y - 1;
		const bool cursorVisible = focused && ((g_millis() - lastInputTime < 300) || cursor.blink) &&
		                           cursorX >= 0 && cursorX < columns && cursorY >= 0 && cursorY < rows;
		g_mutex_release(stateLock);
		// No document/state lock is held during Cairo work or the server's blit acknowledgement.
		Rectangle changes(-1, -1, 0, 0);
		if(full)
		{
			changes = Rectangle(0, 0, columns, rows);
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
			cairo_paint(cr);
			cairo_restore(cr);
		}
		else
		{
			for(int y = 0; y < rows; ++y)
				for(int x = 0; x < columns; ++x)
					if(current[(size_t) y * columns + x] != previous[(size_t) y * columns + x])
						changes.extend(Point(x, y));
		}
		if(lastCursorX >= 0)
			changes.extend(Point(lastCursorX, lastCursorY));
		if(cursorVisible)
			changes.extend(Point(cursorX, cursorY));
		lastCursorX = cursorVisible ? cursorX : -1;
		lastCursorY = cursorVisible ? cursorY : -1;

		if(changes.width > 0 && changes.height > 0)
		{
			const int left = std::max(0, changes.x), top = std::max(0, changes.y);
			const int right = std::min(columns, changes.x + changes.width);
			const int bottom = std::min(rows, changes.y + changes.height);
			Rectangle pixels(left * chars.width + canvas.padding, top * chars.height + canvas.padding,
			                 (right - left) * chars.width, (bottom - top) * chars.height);
			chars.renderer->prepareContext(cr, chars.fontSize);
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
			cairo_rectangle(cr, pixels.x, pixels.y, pixels.width, pixels.height);
			cairo_fill(cr);
			cairo_restore(cr);
			for(int y = top; y < bottom; ++y)
				chars.renderer->printText(cr, left * chars.width + canvas.padding,
				                         y * chars.height + chars.height - 3 + canvas.padding,
				                         current.data() + (size_t) y * columns + left, right - left, chars.width);
			if(cursorVisible)
			{
				cairo_save(cr);
				cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
				cairo_rectangle(cr, cursorX * chars.width + canvas.padding,
				                cursorY * chars.height + canvas.padding, 1, chars.height);
				cairo_fill(cr);
				cairo_restore(cr);
			}
			canvas.component->blit(Rectangle(std::max(0, pixels.x - 2), std::max(0, pixels.y - 2),
			                                pixels.width + 4, pixels.height + 4));
		}
		canvas.component->releaseGraphics();
		previous.swap(current);
		previousColumns = columns;
		previousRows = rows;
	}
}

bool charIsUtf8(char c)
{
	if((c == 0x09 || c == 0x0A || c == 0x0D || (0x20 <= c && c <= 0x7E)))
	{
		return true;
	}
	return false;
}

void gui_screen_t::clean()
{
	g_mutex_acquire(stateLock);
	document->clear();
	fullRepaint = true;
	recalculateView();
	repaint();
	g_mutex_release(stateLock);
}

void gui_screen_t::remove()
{
	g_mutex_acquire(stateLock);
	document->remove(cursor.x, cursor.y);
	recalculateView();
	g_mutex_release(stateLock);
}

void gui_screen_t::backspace()
{
	g_mutex_acquire(stateLock);
	setCursor(std::max(0, cursor.x - 1), cursor.y);
	document->remove(cursor.x, cursor.y);
	recalculateView();
	g_mutex_release(stateLock);
}

void gui_screen_t::recalculateView()
{
	documentRows = document->getRowCount((canvas.bounds.width - 2 * canvas.padding) / chars.width);
	normalizeScroll();
}

void gui_screen_t::write(char c)
{
	write(&c, 1);
}

void gui_screen_t::write(const char* data, size_t length)
{
	g_mutex_acquire(stateLock);
	outputText.clear();
	outputText.reserve(length);
	for(size_t i = 0; i < length; ++i)
		if(charIsUtf8(data[i]))
			outputText.push_back(data[i]);
	if(!outputText.empty())
		documentRows = document->write(cursor.x, cursor.y, outputText.data(), outputText.size());
	cursor.blink = true;
	normalizeScroll();
	g_mutex_release(stateLock);
}


void gui_screen_t::setCursor(int x, int y)
{
	g_mutex_acquire(stateLock);
	cursor.x = x;
	cursor.y = y;
	cursor.blink = true;
	g_mutex_release(stateLock);
}

int gui_screen_t::getCursorX()
{
	g_mutex_acquire(stateLock);
	const int result = cursor.x;
	g_mutex_release(stateLock);
	return result;
}

int gui_screen_t::getCursorY()
{
	g_mutex_acquire(stateLock);
	const int result = cursor.y;
	g_mutex_release(stateLock);
	return result;
}

void gui_screen_t::flush()
{
	repaint();
}

void gui_screen_t::repaint() const
{
	if(!__atomic_exchange_n(&repaintPending, true, __ATOMIC_ACQ_REL))
		g_mutex_release(upToDate);
}

void gui_screen_t::setFocused(bool _focused)
{
	g_mutex_acquire(stateLock);
	lastInputTime = g_millis();
	focused = _focused;
	g_mutex_release(stateLock);
}

int gui_screen_t::getColumns()
{
	g_mutex_acquire(stateLock);
	const int result = (canvas.bounds.width - 2 * canvas.padding) / chars.width;
	g_mutex_release(stateLock);
	return result;
}

int gui_screen_t::getRows()
{
	g_mutex_acquire(stateLock);
	const int result = (canvas.bounds.height - 2 * canvas.padding) / chars.height;
	g_mutex_release(stateLock);
	return result;
}

void gui_screen_t::setCanvasBounds(Rectangle& bounds)
{
	g_mutex_acquire(stateLock);
	canvas.bounds = bounds;
	recalculateView();
	fullRepaint = true;
	g_mutex_release(stateLock);
}

void gui_screen_t::setScrollAreaScreen()
{
	g_mutex_acquire(stateLock);
	scrollY = 0;
	fullRepaint = true;
	normalizeScroll();
	repaint();
	g_mutex_release(stateLock);
}

void gui_screen_t::setScrollArea(int start, int end)
{
	g_mutex_acquire(stateLock);
	scrollY = end;
	fullRepaint = true;
	normalizeScroll();
	repaint();
	g_mutex_release(stateLock);
}

void gui_screen_t::scroll(int value)
{
	g_mutex_acquire(stateLock);
	scrollY = std::max<int64_t>(INT_MIN / 2, std::min<int64_t>(INT_MAX / 2, (int64_t) scrollY + value));
	fullRepaint = true;
	normalizeScroll();
	repaint();
	g_mutex_release(stateLock);
}

void gui_screen_t::normalizeScroll()
{
	int viewRows = (canvas.bounds.height - 2 * canvas.padding) / chars.height;

	if(scrollY > documentRows)
		scrollY = documentRows;

	if(viewRows < documentRows)
	{
		if(scrollY > documentRows - viewRows)
		{
			scrollY = documentRows - viewRows;
		}
		else if(scrollY < -viewRows + 1)
		{
			scrollY = -viewRows + 1;
		}
	}
	else
	{
		scrollY = 0;
	}
}

void gui_screen_t::setCursorVisible(bool visible)
{
	// TODO
}
