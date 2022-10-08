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

#ifndef __WINDOWSERVER_LAYOUT_GRIDLAYOUTMANAGER__
#define __WINDOWSERVER_LAYOUT_GRIDLAYOUTMANAGER__

#include "layout/layout_manager.hpp"
#include <libwindow/metrics/dimension.hpp>
#include <libwindow/metrics/insets.hpp>

class grid_layout_manager_t : public layout_manager_t
{
  private:
    int columns;
    int rows;
    g_insets padding;
    int rowSpace;
    int colSpace;

  public:
    grid_layout_manager_t(int columns = 1, int rows = 0, int rowSpace = 0, int columnSpace = 0)
        : columns(columns), rows(rows), padding(g_insets(0, 0, 0, 0)),
          rowSpace(rowSpace), colSpace(columnSpace)
    {
    }

    void setPadding(g_insets _padding)
    {
        padding = _padding;
    }

    g_insets getPadding() const
    {
        return padding;
    }

    void setRowSpace(int _space)
    {
        rowSpace = _space;
    }

    int getRowSpace() const
    {
        return rowSpace;
    }

    void setColSpace(int _space)
    {
        colSpace = _space;
    }

    int getColSpace() const
    {
        return colSpace;
    }

    virtual void layout();
};

#endif
