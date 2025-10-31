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

#include "layout/flex_layout_manager.hpp"
#include "components/component.hpp"

void flex_layout_manager_t::setLayoutInfo(component_t* child, float grow, float shrink, int basis)
{
	flex_info_t info;
	info.grow = grow;
	info.shrink = shrink;
	info.basis = basis;
	flexInfo[child] = info;
}

void flex_layout_manager_t::layout()
{
	if(component == nullptr)
		return;

	g_rectangle bounds = component->getBounds();
	bounds.x = padding.left;
	bounds.y = padding.top;
	bounds.width -= padding.left + padding.right;
	bounds.height -= padding.top + padding.bottom;

	auto& children = component->acquireChildren();
	int totalSpace = horizontal ? bounds.width : bounds.height;
	int totalFlexGrow = 0;
	int totalFixedSpace = 0;

	// Calculate required space for fixed-size
	for(auto& ref: children)
	{
		component_t* child = ref.component;
		flex_info_t flex = flexInfo[child];

		int basis = (flex.basis >= 0)
			            ? flex.basis
			            : (horizontal ? child->getPreferredSize().width : child->getPreferredSize().height);
		totalFixedSpace += basis;
		totalFlexGrow += flex.grow;
	}

	// Calculate remaining space, considering the gaps
	int remainingSize = totalSpace - totalFixedSpace - (gap * (children.size() - 1));

	// Distribute space
	int xOffset = bounds.x;
	int yOffset = bounds.y;

	for(auto& ref: children)
	{
		component_t* child = ref.component;
		flex_info_t flex = flexInfo[child];

		int basis = (flex.basis >= 0)
			            ? flex.basis
			            : (horizontal ? child->getPreferredSize().width : child->getPreferredSize().height);
		int allocatedSize = basis;

		if(remainingSize > 0 && flex.grow > 0)
		{
			float proportion = flex.grow / static_cast<float>(totalFlexGrow);
			allocatedSize += static_cast<int>(proportion * remainingSize);
		}

		if(horizontal)
		{
			child->setBounds(g_rectangle(xOffset, yOffset, allocatedSize, bounds.height));
			xOffset += allocatedSize + gap; // Add the gap after each component
		}
		else
		{
			child->setBounds(g_rectangle(xOffset, yOffset, bounds.width, allocatedSize));
			yOffset += allocatedSize + gap; // Add the gap after each component
		}
	}

	component->releaseChildren();
}
