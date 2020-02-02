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

#include <components/component.hpp>
#include <vector>

#include <ghostuser/utils/logger.hpp>
#include <layout/flow_layout_manager.hpp>

#include <typeinfo>

/**
 *
 */
void flow_layout_manager_t::layout() {

	if (component == 0) {
		return;
	}

	std::vector<component_child_reference_t>& children = component->getChildren();

	int x = 0;
	int y = 0;
	int lineHeight = 0;

	g_rectangle parentBounds = component->getBounds();
	for (auto& ref : children) {
		component_t* c = ref.component;

		g_dimension preferredSize = c->getPreferredSize();

		if (x + preferredSize.width > parentBounds.width) {
			x = 0;
			y += lineHeight;
			lineHeight = 0;
		}

		c->setBounds(g_rectangle(x, y, preferredSize.width, preferredSize.height));
		x += preferredSize.width;

		if (preferredSize.height > lineHeight) {
			lineHeight = preferredSize.height;
		}

	}
}
