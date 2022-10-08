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

#ifndef __LIBWINDOW_BOUNDS_EVENT_COMPONENT__
#define __LIBWINDOW_BOUNDS_EVENT_COMPONENT__

#include <cstdint>
#include <ghost.h>

#include "libwindow/interface.hpp"
#include "libwindow/listener/bounds_listener.hpp"

/**
 * Component that is capable of receiving bounds events
 */
class g_bounds_event_component
{
  private:
	g_ui_component_id id;

  protected:
	g_component *self;

	g_bounds_event_component(g_component *self, g_ui_component_id id) : self(self), id(id)
	{
	}

  public:
	virtual ~g_bounds_event_component()
	{
	}

	bool setBoundsListener(g_bounds_listener *l);
};

#endif
