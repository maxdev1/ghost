/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef ITEM_CONTAINER_HPP
#define ITEM_CONTAINER_HPP

#include <components/component.hpp>

#include "selection.hpp"
#include "item.hpp"


class item_container_t : public component_t
{
	int gridScale = 100;
	selection_t* selection;

public:
	item_container_t();
	~item_container_t() override = default;

	int getGridScale() const { return gridScale; }

	void updateSelectedChildren(const g_rectangle& newSelection);
	void showSelection(g_rectangle& selection);
	void hideSelection();
	void startLoadDesktopItems();
	void unselectItems();
	void setSelectedItem(item_t* item);
	void pressDesktopItems(const g_point& screen_position);
	void dragDesktopItems(const g_point& screen_position);
	void tidyDesktopItems();
};


#endif //ITEM_CONTAINER_HPP
