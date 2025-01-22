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


#include "item_container.hpp"

item_container_t::item_container_t()
{
	selection = new selection_t();
	selection->setVisible(false);
	component_t::addChild(selection);
}

void item_container_t::startLoadDesktopItems()
{
	item_t* terminal = new item_t(this, "Terminal", "/applications/terminal.bin",
	                              "/system/graphics/app-icons/terminal.png");
	terminal->setBounds(g_rectangle(gridScale, gridScale, gridScale, gridScale));
	this->addChild(terminal);

	item_t* calculator = new item_t(this, "Calculator", "/applications/calculator.bin",
	                                "/system/graphics/app-icons/calculator.png");
	calculator->setBounds(g_rectangle(gridScale, gridScale * 2, gridScale, gridScale));
	this->addChild(calculator);
}


void item_container_t::showSelection(g_rectangle& newSelection)
{
	if(newSelection.width != 0 || newSelection.height != 0)
	{
		selection->setBounds(newSelection.asNormalized());
		selection->setVisible(true);
		updateSelectedChildren(newSelection.asNormalized());
	}
	else
	{
		selection->setVisible(false);
		unselectItems();
	}
}

void item_container_t::updateSelectedChildren(const g_rectangle& newSelection)
{
	g_mutex_acquire(this->getChildrenLock());
	for(auto child: this->getChildren())
	{
		bool intersecting = newSelection.intersects(child.component->getBounds());
		auto childDesktopItem = dynamic_cast<item_t*>(child.component);
		if(childDesktopItem)
			childDesktopItem->setSelected(intersecting);
	}
	g_mutex_release(this->getChildrenLock());
}


void item_container_t::hideSelection() { selection->setVisible(false); }

void item_container_t::unselectItems()
{
	g_mutex_acquire(this->getChildrenLock());
	for(auto child: this->getChildren())
	{
		auto childDesktopItem = dynamic_cast<item_t*>(child.component);
		if(childDesktopItem)
			childDesktopItem->setSelected(false);
	}
	g_mutex_release(this->getChildrenLock());
}

void item_container_t::setSelectedItem(item_t* item)
{
	unselectItems();
	item->setSelected(true);
}

void item_container_t::pressDesktopItems(const g_point& screenPosition)
{
	g_mutex_acquire(this->getChildrenLock());
	for(auto child: this->getChildren())
	{
		auto childDesktopItem = dynamic_cast<item_t*>(child.component);
		if(childDesktopItem)
			childDesktopItem->onContainerItemPressed(screenPosition);
	}
	g_mutex_release(this->getChildrenLock());
}

void item_container_t::dragDesktopItems(const g_point& screenPosition)
{
	g_mutex_acquire(this->getChildrenLock());
	for(auto child: this->getChildren())
	{
		auto childDesktopItem = dynamic_cast<item_t*>(child.component);
		if(childDesktopItem && childDesktopItem->isSelected())
		{
			childDesktopItem->onContainerItemDragged(screenPosition);
		}
	}
	g_mutex_release(this->getChildrenLock());
}

void item_container_t::tidyDesktopItems()
{
	g_mutex_acquire(this->getChildrenLock());
	for(auto child: this->getChildren())
	{
		auto childDesktopItem = dynamic_cast<item_t*>(child.component);
		if(childDesktopItem && childDesktopItem->isSelected())
		{
			childDesktopItem->tidyPosition();
		}
	}
	g_mutex_release(this->getChildrenLock());

}
