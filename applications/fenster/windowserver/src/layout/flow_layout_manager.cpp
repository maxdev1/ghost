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

#include "layout/flow_layout_manager.hpp"
#include "components/component.hpp"

/**
 *
 */
void flow_layout_manager_t::layout()
{
	if(component == nullptr)
		return;


	g_rectangle bounds = component->getBounds();
	bounds.x = padding.left;
	bounds.y = padding.top;
	bounds.width -= padding.left + padding.right;
	bounds.height -= padding.top + padding.bottom;

	int x = bounds.x;
	int y = bounds.y;
	int rowHeight = 0;

	auto& children = component->acquireChildren();
	for(auto& childRef: children)
	{
		component_t* child = childRef.component;
		if(!child->isVisible())
			continue;
		g_dimension childSize = child->getEffectivePreferredSize();

		// Break when reaching end
		if(x + childSize.width > bounds.width)
		{
			x = bounds.x;
			y += rowHeight;
			rowHeight = 0;
		}

		// Set size
		child->setBounds(g_rectangle(x, y, childSize.width, childSize.height));
		x += childSize.width;

		if(childSize.height > rowHeight)
			rowHeight = childSize.height;
	}
	component->releaseChildren();

	component->setPreferredSize(g_dimension(bounds.width == 0 ? x : bounds.width, y + rowHeight));
}
