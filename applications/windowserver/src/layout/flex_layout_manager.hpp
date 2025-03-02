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

#ifndef __FLEX_LAYOUT_MANAGER__
#define __FLEX_LAYOUT_MANAGER__

#include "layout_manager.hpp"
#include <unordered_map>
#include <libwindow/metrics/insets.hpp>

struct flex_info_t
{
    float grow;
    float shrink;
    int basis;
};

class flex_layout_manager_t : public layout_manager_t
{
    std::unordered_map<component_t*, flex_info_t> flexInfo;
    bool horizontal = true;
    g_insets padding = g_insets(0, 0, 0, 0);
    int gap = 0;

public:
    void layout() override;

    void setLayoutInfo(component_t* child, float grow, float shrink, int basis);

    void setHorizontal(bool horizontal)
    {
        this->horizontal = horizontal;
    }

    int getGap()
    {
        return gap;
    }

    void setGap(int gap)
    {
        this->gap = gap;
    }

    void setPadding(g_insets padding) override
    {
        this->padding = padding;
    }
};

#endif
