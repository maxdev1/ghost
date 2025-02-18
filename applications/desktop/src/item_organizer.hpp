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


#ifndef DESKTOP_ITEM_ORGANIZER
#define DESKTOP_ITEM_ORGANIZER

#include "item.hpp"

#include <vector>

class item_organizer_t
{
    struct
    {
        int cellWidth;
        int cellHeight;
        int xPadding;
        int yPadding;
    } grid;

public:
    item_organizer_t(int cellWidth, int cellHeight, int xPadding, int yPadding) :
        grid{cellWidth, cellHeight, xPadding, yPadding}
    {
    }

    void organize(const std::vector<item_t*>& items, const g_rectangle& backgroundBounds) const;
};


#endif
