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

#ifndef GHOSTLIBRARY_UI_TITLED_COMPONENT
#define GHOSTLIBRARY_UI_TITLED_COMPONENT

#include <cstdint>
#include <ghost.h>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/utils/value_placer.hpp>

/**
 *
 */
class g_titled_component {
private:
	uint32_t id;

protected:
	g_titled_component(uint32_t id) :
			id(id) {
	}

public:
	/**
	 *
	 */
	virtual ~g_titled_component() {
	}

	/**
	 *
	 */
	virtual bool setTitle(std::string title);

	/**
	 *
	 */
	virtual std::string getTitle();

};

#endif
