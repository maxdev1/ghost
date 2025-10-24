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

#ifndef __LIBWINDOW_METRICS_RECTANGLE__
#define __LIBWINDOW_METRICS_RECTANGLE__

#include "dimension.hpp"
#include "insets.hpp"
#include "point.hpp"
#include <cstdint>
#include <algorithm>

struct g_rectangle
{
public:
    int x;
    int y;
    int width;
    int height;

    g_rectangle() :
        x(0), y(0), width(0), height(0)
    {
    }

    g_rectangle(int x, int y, int width, int height) :
        x(x), y(y), width(width), height(height)
    {
    }

    g_rectangle(const g_rectangle& rhs) :
        x(rhs.x), y(rhs.y), width(rhs.width), height(rhs.height)
    {
    }

    g_rectangle& operator=(const g_rectangle& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        width = rhs.width;
        height = rhs.height;
        return *this;
    }

    bool operator==(const g_rectangle& rhs) const
    {
        return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
    }

    bool operator!=(const g_rectangle& rhs) const
    {
        return !(*this == rhs);
    }

    g_rectangle& operator-=(const g_insets& rhs)
    {
        x = x + rhs.left;
        y = y + rhs.top;
        width = width - rhs.left - rhs.right;
        height = height - rhs.top - rhs.bottom;
        return *this;
    }

    bool contains(g_point p) const
    {
        return (p.x >= x) && (p.y >= y) && (p.x < x + width) && (p.y < y + height);
    }

    int getTop() const
    {
        return y;
    }

    void setTop(int _top)
    {
        y = _top;
    }

    int getLeft() const
    {
        return x;
    }

    void setLeft(int _left)
    {
        x = _left;
    }

    int getBottom() const
    {
        return y + height;
    }

    void setBottom(int32_t _bottom)
    {
        height = _bottom - y;
    }

    int getRight() const
    {
        return x + width;
    }

    void setRight(int _right)
    {
        width = _right - x;
    }

    void setStart(g_point p)
    {
        x = p.x;
        y = p.y;
    }

    g_point getStart() const
    {
        return g_point(x, y);
    }

    void setEnd(g_point p)
    {
        width = p.x - x;
        height = p.y - y;
    }

    g_point getEnd() const
    {
        return g_point(x + width, y + height);
    }

    g_rectangle asNormalized()
    {
        g_rectangle norm = *this;

        if (norm.width < 0)
        {
            norm.x += norm.width;
            norm.width = -norm.width;
        }
        if (norm.height < 0)
        {
            norm.y += norm.height;
            norm.height = -norm.height;
        }
        return norm;
    }

    g_dimension getSize()
    {
        return g_dimension(width, height);
    }

    bool intersects(g_rectangle p) const
    {
        return !(x + width <= p.x || p.x + p.width <= x || y + height <= p.y || p.y + p.height <= y);
    }

    /**
     * Extends this rectangle so the point is included.
     */
    void extend(const g_point& p)
    {
        if (width <= 0 || height <= 0)
        {
            x = p.x;
            y = p.y;
            width = 1;
            height = 1;
            return;
        }

        int right = std::max(x + width, p.x + 1);
        int bottom = std::max(y + height, p.y + 1);
        x = std::min(x, p.x);
        y = std::min(y, p.y);
        width = right - x;
        height = bottom - y;
    }

    /**
     * Clips this rectangle to be within the given rectangle.
     */
    [[nodiscard]] g_rectangle clip(const g_rectangle& other) const
    {
        // Find the intersecting rectangle coordinates
        int left = std::max(x, other.x);
        int top = std::max(y, other.y);
        int right = std::min(x + width, other.x + other.width);
        int bottom = std::min(y + height, other.y + other.height);

        // If there's no intersection, return a rectangle with zero dimensions
        if (left >= right || top >= bottom)
        {
            return {0, 0, 0, 0};
        }

        // Return the intersecting rectangle
        return {
            left,
            top,
            right - left,
            bottom - top
        };
    }
} __attribute__((packed));

#endif
