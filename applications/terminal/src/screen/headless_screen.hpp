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

class headless_screen_t : public screen_t {
private:
	uint32_t offset;

	g_user_mutex lock;

	void normalize();

	void enableCursor();

	void updateCursor();

public:
	headless_screen_t();

	bool initialize() override;

	g_key_info readInput() override;

	void clean() override;

	void backspace() override;

	void write(char c) override;

	void flush() override
	{
	}

	void remove() override;

	void setCursor(int x, int y) override;

	int getCursorX() override;

	int getCursorY() override;

	void setCursorVisible(bool visible) override
	{
	}

	void setScrollAreaScreen() override
	{
	}

	void setScrollArea(int start, int end) override
	{
	}

	void scroll(int value) override
	{
	}

	int getColumns() override;

	int getRows() override;
};

#endif
