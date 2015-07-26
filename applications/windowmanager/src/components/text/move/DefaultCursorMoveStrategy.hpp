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

#ifndef SIMPLESOFTCHARSKIPSTRATEGY_HPP_
#define SIMPLESOFTCHARSKIPSTRATEGY_HPP_

#include <components/text/move/CursorMoveStrategy.hpp>

/*
 *
 */
class DefaultCursorMoveStrategy: public CursorMoveStrategy {
private:
	DefaultCursorMoveStrategy() {
	}

public:
	virtual ~DefaultCursorMoveStrategy() {
	}

	/**
	 *
	 */
	virtual void moveCursor(TextComponent* component, CursorDirection direction, g_key_info& keyInfo);

	/**
	 *
	 */
	virtual int calculateSkip(std::string text, int position, CursorDirection direction);

	/**
	 *
	 */
	static DefaultCursorMoveStrategy* getInstance();
};

#endif
