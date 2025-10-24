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

#include "layout/stack_layout_manager.hpp"
#include "components/component.hpp"

/**
 *
 */
void stack_layout_manager_t::layout()
{
	if(component == nullptr)
		return;

	if(horizontal)
	{
		int x = 0;
		int highestHeight = 0;

		auto& children = component->acquireChildren();
		for(auto& childRef: children)
		{
			component_t* child = childRef.component;
			if(!child->isVisible())
				continue;
			g_dimension childSize = child->getEffectivePreferredSize();

			child->setBounds(g_rectangle(x, 0, childSize.width, childSize.height));
			x += childSize.width;

			highestHeight = childSize.height > highestHeight ? childSize.height : highestHeight;
		}
		component->releaseChildren();

		component->setPreferredSize(g_dimension(x, highestHeight));
	}
	else
	{
		int y = 0;
		int widestWidth = 0;

		auto& children = component->acquireChildren();
		for(auto& childRef: children)
		{
			component_t* child = childRef.component;
			if(!child->isVisible())
				continue;
			g_dimension childSize = child->getEffectivePreferredSize();

			child->setBounds(g_rectangle(0, y, childSize.width, childSize.height));
			y += childSize.height;

			widestWidth = childSize.width > widestWidth ? childSize.width : widestWidth;
		}
		component->releaseChildren();

		component->setPreferredSize(g_dimension(widestWidth, y));
	}
}
