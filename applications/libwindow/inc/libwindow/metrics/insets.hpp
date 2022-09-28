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

#ifndef __LIBWINDOW_METRICS_INSETS__
#define __LIBWINDOW_METRICS_INSETS__

#include "libwindow/metrics/point.hpp"
#include <cstdint>

class g_insets
{
  public:
    int top;
    int left;
    int bottom;
    int right;

    g_insets() : top(0), left(0), bottom(0), right(0)
    {
    }

    g_insets(int top, int left, int bottom, int right) : top(top), left(left), bottom(bottom), right(right)
    {
    }

    g_insets& operator=(const g_insets& rhs)
    {
        top = rhs.top;
        left = rhs.left;
        bottom = rhs.bottom;
        right = rhs.right;
        return *this;
    }

    bool operator==(const g_insets& rhs) const
    {
        return top == rhs.top && left == rhs.left && bottom == rhs.bottom && right == rhs.right;
    }

    bool operator!=(const g_insets& rhs) const
    {
        return !(*this == rhs);
    }
};

#endif
