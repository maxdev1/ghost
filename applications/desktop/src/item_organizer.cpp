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


#include "item_organizer.hpp"

void item_organizer::organize(const std::vector<item*>& items, const g_rectangle& backgroundBounds) const
{
	std::vector<g_rectangle> allBounds;
	for(auto& item: items)
	{
		auto bounds = item->getBounds();
		allBounds.push_back(bounds);
	}

	int cw = grid.cellWidth;
	int ch = grid.cellHeight;
	int maxX = backgroundBounds.width / cw - (backgroundBounds.width % cw == 0 ? 0 : 1);
	int maxY = backgroundBounds.height / ch - (backgroundBounds.height % ch == 0 ? 0 : 1);

	uint8_t taken[(maxX + 1) * (maxY + 1)] = {};
	std::vector<int> resortIndexes;

	for(int i = 0; i < items.size(); i++)
	{
		auto& bounds = allBounds[i];

		int gridX = (bounds.x + cw / 2) / cw;
		int gridY = (bounds.y + ch / 2) / ch;

		if(gridX < 0)
			gridX = 0;
		if(gridY < 0)
			gridY = 0;
		if(gridX > maxX)
			gridX = maxX;
		if(gridY > maxY)
			gridY = maxY;

		bounds.x = gridX * cw;
		bounds.y = gridY * ch;

		if(taken[gridY * maxX + gridX])
		{
			resortIndexes.push_back(i);
		}
		else
		{
			taken[gridY * maxX + gridX] = 1;
		}
	}

	for(int r = 0; r < resortIndexes.size(); r++)
	{
		int i = resortIndexes[r];
		for(int t = 0; t < maxX * maxY; t++)
		{
			if(!taken[t])
			{
				allBounds[i].x = (t % maxX) * cw;
				allBounds[i].y = (t / maxX) * ch;
				taken[t] = 1;
				break;
			}
		}
	}

	for(int i = 0; i < items.size(); i++)
	{
		auto& bounds = allBounds[i];
		items[i]->setBounds(bounds);
	}
}
