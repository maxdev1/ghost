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

#include <ghostuser/graphics/Graphics.hpp>
#include <FPSCounterThread.hpp>
#include <WindowManager.hpp>
#include <ghost.h>
#include <sstream>

static uint32_t ticks = 0;
static uint32_t fps = 0;

/**
 *
 */
void FPSCounterThread::run() {
	while (true) {
		fps = ticks;
		ticks = 0;
		g_sleep(1000);
	}
}

/**
 *
 */
void FPSCounterThread::tick() {
	++ticks;

	std::stringstream str;
	str << fps;
	WindowManager::getFpsLabel()->setTitle(str.str());
}

