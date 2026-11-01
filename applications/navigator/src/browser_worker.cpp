/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <ghost.h>
#include "browser_window.hpp"
#include "work_queue.hpp"

namespace
{
	g_user_mutex workLock = g_mutex_initialize();
	g_user_mutex workReady = g_mutex_initialize();
	navigator::WorkQueue workQueue;

	void navigatorBrowserWorker()
	{
		for(;;)
		{
			g_mutex_acquire(workReady);
			for(;;)
			{
				g_mutex_acquire(workLock);
				auto work = workQueue.queuePop();
				g_mutex_release(workLock);
				if(!work)
					break;
				work();
			}
		}
	}
} // namespace

bool navigatorStartBrowserWorker()
{
	g_mutex_acquire(workReady);
	return g_create_task(reinterpret_cast<void*>(navigatorBrowserWorker)) != G_TID_NONE;
}

void navigatorEnqueue(std::function<void()> work, const void* owner, int key)
{
	g_mutex_acquire(workLock);
	workQueue.queuePush(std::move(work), owner, key);
	g_mutex_release(workLock);
	g_mutex_release(workReady);
}
