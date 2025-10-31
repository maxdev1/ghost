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

#include "process_registry.hpp"

#include <map>

static std::map<SYS_TID_T, SYS_TID_T> remoteDelegates;
static SYS_MUTEX_T remoteDelegatesLock = platformInitializeMutex(false);

void process_registry_t::bind(SYS_TID_T pid, SYS_TID_T tid)
{
	platformAcquireMutex(remoteDelegatesLock);
	remoteDelegates[pid] = tid;
	platformReleaseMutex(remoteDelegatesLock);
}

SYS_TID_T process_registry_t::get(SYS_TID_T pid)
{
	platformAcquireMutex(remoteDelegatesLock);
	if(remoteDelegates.count(pid) > 0)
	{
		SYS_TID_T tid = remoteDelegates[pid];
		platformReleaseMutex(remoteDelegatesLock);
		return tid;
	}
	platformReleaseMutex(remoteDelegatesLock);
	return SYS_TID_NONE;
}

void process_registry_t::cleanup_process(SYS_TID_T pid)
{
	platformAcquireMutex(remoteDelegatesLock);
	remoteDelegates.erase(pid);
	platformReleaseMutex(remoteDelegatesLock);
}
