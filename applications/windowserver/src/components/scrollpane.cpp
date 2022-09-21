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
#include "events/mouse_event.hpp"
#include <stdio.h>

#define SCROLLBAR_SIZE 15

scrollpane_t::scrollpane_t()
{
}

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

void scrollpane_t::layout()
{
    auto bounds = getBounds();

    int viewportHeight = bounds.height;
    int viewportWidth = bounds.width;
    auto contentBounds = content->getPreferredSize();

    bool showVbar = false;
    bool showHbar = false;

    if(contentBounds.height > viewportHeight)
    {
        showVbar = true;
    }
    if(contentBounds.width > (showVbar ? viewportWidth - SCROLLBAR_SIZE : viewportWidth))
    {
        showHbar = true;
    }
    if(contentBounds.height > (showHbar ? viewportHeight - SCROLLBAR_SIZE : viewportHeight))
    {
        showVbar = true;
    }
    if(contentBounds.width > (showVbar ? viewportWidth - SCROLLBAR_SIZE : viewportWidth))
    {
        showHbar = true;
    }

    if(showHbar)
    {
        viewportHeight -= SCROLLBAR_SIZE;
    }
    if(showVbar)
    {
        viewportWidth -= SCROLLBAR_SIZE;
    }

    if(contentBounds.height > viewportHeight)
    {
        verticalScrollbar.setViewLengths(viewportHeight, contentBounds.height);
        verticalScrollbar.setBounds(g_rectangle(viewportWidth, 0, SCROLLBAR_SIZE, viewportHeight));
        verticalScrollbar.setVisible(true);
    }
    else
    {
        verticalScrollbar.setVisible(false);
    }

    if(contentBounds.width > viewportWidth)
    {
        horizontalScrollbar.setViewLengths(viewportWidth, contentBounds.width);
        horizontalScrollbar.setBounds(g_rectangle(0, viewportHeight, viewportWidth, SCROLLBAR_SIZE));
        horizontalScrollbar.setVisible(true);
    }
    else
    {
        horizontalScrollbar.setVisible(false);
    }

    updateContent();

    markFor(COMPONENT_REQUIREMENT_PAINT);
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

void scrollpane_t::updateContent()
{
    if(!content)
        return;

    g_dimension contentSize = content->getPreferredSize();
    g_rectangle viewportBounds = getBounds();
    viewportBounds.width -= SCROLLBAR_SIZE;
    viewportBounds.height -= SCROLLBAR_SIZE;

    if(scrollPosition.x > 0)
    {
        scrollPosition.x = 0;
    }
    else if(contentSize.width < viewportBounds.width)
    {
        scrollPosition.x = 0;
    }
    else if(scrollPosition.x + contentSize.width < viewportBounds.width)
    {
        scrollPosition.x = viewportBounds.width - contentSize.width;
    }

    if(scrollPosition.y > 0)
    {
        scrollPosition.y = 0;
    }
    else if(contentSize.height < viewportBounds.height)
    {
        scrollPosition.y = 0;
    }
    else if(scrollPosition.y + contentSize.height < viewportBounds.height)
    {
        scrollPosition.y = viewportBounds.height - contentSize.height;
    }

    content->setBounds(g_rectangle(scrollPosition.x, scrollPosition.y, contentSize.width, contentSize.height));
}
