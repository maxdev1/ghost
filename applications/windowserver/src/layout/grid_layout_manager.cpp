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

#include "layout/grid_layout_manager.hpp"
#include "components/component.hpp"

#include <typeinfo>
#include <vector>

void grid_layout_manager_t::layout()
{
	if(component == 0)
		return;

	g_rectangle usedBounds = component->getBounds();

	usedBounds.x = padding.left;
	usedBounds.y = padding.top;
	usedBounds.width -= padding.left + padding.right;
	usedBounds.height -= padding.top + padding.bottom;

	int rowHeight = 0;
	int cellWidth = (columns > 0) ? (usedBounds.width / columns) : usedBounds.width;
	int itemWidth = (columns > 0) ? ((usedBounds.width - (columns - 1) * colSpace) / columns) : usedBounds.width;

	g_atomic_lock(component->getChildrenLock());
	auto children = component->getChildren();

	int x = usedBounds.x;
	int y = usedBounds.y;
	int curRow = 0;
	int curCol = 0;
	for(auto& ref : children)
	{
		component_t* child = ref.component;

		int itemHeight = ((rows > 0) ? ((usedBounds.height - (rows - 1) * rowSpace) / rows) : child->getPreferredSize().height);

		int xoff = (columns > 0) ? (((float) curCol / columns) * colSpace) : 0;
		int yoff = (rows > 0) ? (((float) curRow / rows) * rowSpace) : 0;
		child->setBounds(g_rectangle(x + xoff, y + yoff, itemWidth, itemHeight));

		x += cellWidth;
		curCol++;

		int cellHeight = ((rows > 0) ? (usedBounds.height / rows) : (child->getPreferredSize().height + rowSpace));
		if(cellHeight > rowHeight)
		{
			rowHeight = cellHeight;
		}
		if(x + cellWidth > usedBounds.x + usedBounds.width)
		{
			x = usedBounds.x;
			y += rowHeight;
			rowHeight = 0;
			curRow++;
			curCol = 0;
		}
	}
	g_atomic_unlock(component->getChildrenLock());

	auto prefPreferred = component->getPreferredSize();

	int addedHeight = rowHeight + padding.bottom;
	if(rows == 0)
	{
		addedHeight -= rowSpace;
	}
	auto newPref = g_dimension(x, y + addedHeight);

	if(prefPreferred != newPref)
	{
		component->setPreferredSize(newPref);
		component->markParentFor(COMPONENT_REQUIREMENT_LAYOUT);
	}
}
