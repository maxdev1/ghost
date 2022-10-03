/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_COMPONENTS_CANVAS__
#define __WINDOWSERVER_COMPONENTS_CANVAS__

#include "components/component.hpp"

struct buffer_info_t
{
	uint8_t* localMapping;
	uint8_t* remoteMapping;
	uint16_t pages;
	bool acknowledged;
};

class canvas_t;
struct async_resizer_info_t
{
	bool alive;
	g_atom lock;
	g_atom checkAtom;
	canvas_t* canvas;
};

class canvas_t : public component_t
{
  public:
	g_pid partnerProcess;
	g_tid partnerThread;

	async_resizer_info_t* asyncInfo;

	g_atom currentBufferLock = g_atomic_initialize();
	buffer_info_t currentBuffer;
	buffer_info_t nextBuffer;

	canvas_t(g_tid partnerThread);
	virtual ~canvas_t();

	virtual void paint();

	virtual bool handle()
	{
		return false;
	}

	virtual void handleBoundChange(g_rectangle oldBounds);

	static void asyncBufferResizer(async_resizer_info_t* info);

	void createNewBuffer(uint16_t requiredPages);
	void clientHasAcknowledgedCurrentBuffer();
	void requestClientToAcknowledgeNewBuffer();
	void blit();

  private:
	void checkBuffer();
};

#endif
