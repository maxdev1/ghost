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

#ifndef SCREEN_HPP_
#define SCREEN_HPP_

#include <components/Component.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>

/**
 *
 */
class screen_t: public Component {
private:

	/**
	 * Area that is invalid and needs to be copied to the graphics buffer
	 */
	g_rectangle invalid;

public:

	/**
	 *
	 */
	virtual ~screen_t() {
	}

	/**
	 * Overrides the default invalidation method. On the component, this method
	 * just dispatches to the parent, but here we must remember the invalidation.
	 */
	virtual void markDirty(g_rectangle rect);

	/**
	 *
	 */
	g_rectangle grabInvalid() {
		g_rectangle ret = invalid;
		invalid = g_rectangle();
		return ret;
	}

};

#endif
