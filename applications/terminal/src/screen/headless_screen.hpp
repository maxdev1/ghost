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

#ifndef __TERMINAL_SCREEN_HEADLESSSCREEN__
#define __TERMINAL_SCREEN_HEADLESSSCREEN__

#include "screen.hpp"

class headless_screen_t : public screen_t
{
  private:
	uint32_t offset;

	g_atom lock;
	void normalize();

	void enableCursor();
	void updateCursor();

  public:
	headless_screen_t();

	virtual bool initialize();

	virtual g_key_info readInput();
	virtual void clean();
	virtual void backspace();
	virtual void write(char c);
	virtual void flush() {}

	virtual void setCursor(int x, int y);
	virtual int getCursorX();
	virtual int getCursorY();
	virtual void setCursorVisible(bool visible) {}

	virtual void setScrollAreaScreen() {}
	virtual void setScrollArea(int start, int end) {}
	virtual void scroll(int value) {}

	virtual int getWidth();
	virtual int getHeight();
};

#endif
