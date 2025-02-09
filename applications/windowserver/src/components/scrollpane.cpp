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

#include "components/scrollpane.hpp"

#define SCROLLBAR_SIZE 15

void scrollpane_t::setContent(component_t* component)
{
	if(this->content)
		removeChild(this->content);

	this->content = component;
	addChild(this->content);

	addChild(&horizontalScrollbar, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	horizontalScrollbar.setScrollHandler(this);

	addChild(&verticalScrollbar, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	verticalScrollbar.setScrollHandler(this);
}

void scrollpane_t::handleScroll(scrollbar_t* bar)
{

	if(bar == &verticalScrollbar)
	{
		scrollPosition.y = -verticalScrollbar.getModelPosition();
		updateContent();
	}
	else if(bar == &horizontalScrollbar)
	{
		scrollPosition.x = -horizontalScrollbar.getModelPosition();
		updateContent();
	}
}

g_dimension scrollpane_t::calculateViewport(g_dimension contentSize)
{
	showVbar = false;
	showHbar = false;

	g_dimension viewportSize = getBounds().getSize();

	if(contentSize.height > viewportSize.height)
	{
		showVbar = true;
	}
	if(contentSize.width > (showVbar ? viewportSize.width - SCROLLBAR_SIZE : viewportSize.width))
	{
		showHbar = true;
	}
	if(contentSize.height > (showHbar ? viewportSize.height - SCROLLBAR_SIZE : viewportSize.height))
	{
		showVbar = true;
	}
	if(contentSize.width > (showVbar ? viewportSize.width - SCROLLBAR_SIZE : viewportSize.width))
	{
		showHbar = true;
	}

	if(showHbar)
	{
		viewportSize.height -= SCROLLBAR_SIZE;
	}
	if(showVbar)
	{
		viewportSize.width -= SCROLLBAR_SIZE;
	}
	return viewportSize;
}

void scrollpane_t::layout()
{
	if(!content)
		return;

	auto contentSize = content->getPreferredSize();
	if(fixedWidth)
	{
		contentSize.width = getBounds().width - SCROLLBAR_SIZE;
	}
	if(fixedHeight)
	{
		contentSize.height = getBounds().height - SCROLLBAR_SIZE;
	}
	auto viewportSize = calculateViewport(contentSize);

	auto bounds = getBounds();
	if(showVbar)
	{
		verticalScrollbar.setViewLengths(viewportSize.height, contentSize.height);
		verticalScrollbar.setBounds(g_rectangle(bounds.width - SCROLLBAR_SIZE, 0, SCROLLBAR_SIZE, viewportSize.height));
		verticalScrollbar.setVisible(true);
	}
	else
	{
		verticalScrollbar.setVisible(false);
	}

	if(showHbar)
	{
		horizontalScrollbar.setViewLengths(viewportSize.width, contentSize.width);
		horizontalScrollbar.setBounds(
				g_rectangle(0, bounds.height - SCROLLBAR_SIZE, viewportSize.width, SCROLLBAR_SIZE));
		horizontalScrollbar.setVisible(true);
	}
	else
	{
		horizontalScrollbar.setVisible(false);
	}

	updateContent();

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

void scrollpane_t::updateContent()
{
	if(!content)
		return;

	auto contentSize = content->getPreferredSize();
	if(fixedWidth)
	{
		contentSize.width = getBounds().width;
	}
	if(fixedHeight)
	{
		contentSize.height = getBounds().height;
	}
	auto viewportSize = calculateViewport(contentSize);

	if(scrollPosition.x > 0)
	{
		scrollPosition.x = 0;
	}
	else if(contentSize.width < viewportSize.width)
	{
		scrollPosition.x = 0;
	}
	else if(scrollPosition.x + contentSize.width < viewportSize.width)
	{
		scrollPosition.x = viewportSize.width - contentSize.width;
	}

	if(scrollPosition.y > 0)
	{
		scrollPosition.y = 0;
	}
	else if(contentSize.height < viewportSize.height)
	{
		scrollPosition.y = 0;
	}
	else if(scrollPosition.y + contentSize.height < viewportSize.height)
	{
		scrollPosition.y = viewportSize.height - contentSize.height;
	}

	content->setBounds(g_rectangle(scrollPosition.x, scrollPosition.y,
	                               fixedWidth ? viewportSize.width : contentSize.width,
	                               fixedHeight ? viewportSize.height : contentSize.height));
}
