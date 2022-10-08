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

#ifndef __WINDOWSERVER_COMPONENTS_BACKGROUND__
#define __WINDOWSERVER_COMPONENTS_BACKGROUND__

#include "components/component.hpp"
#include "components/desktop/desktop_item.hpp"
#include <libwindow/metrics/rectangle.hpp>

class background_t : public component_t
{
  private:
	cairo_surface_t* surface = 0;
	g_rectangle selection;
	desktop_item_t* selectedItem = nullptr;

  public:
	int gridScale = 100;

	virtual ~background_t()
	{
	}

	virtual void paint();

	virtual void load(const char* path);

	void showSelection(g_rectangle& selection);
	void hideSelection();
	void startLoadDesktopItems();
	void setSelectedItem(desktop_item_t* item);
	desktop_item_t* getSelectedItem();
};

#endif
