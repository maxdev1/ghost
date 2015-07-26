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

#ifndef PLAINCONSOLEPANEL_HPP_
#define PLAINCONSOLEPANEL_HPP_

#include <components/Component.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/graphics/text/font.hpp>
#include <string>

/**
 *
 */
class PlainConsolePanel: public Component {
private:
	g_font* font;
	g_layouted_text viewModel;
	std::string content;
	bool focused;

public:
	PlainConsolePanel();

	virtual void update();
	virtual void paint();
	virtual bool handle(Event& e);

	void append(char c);
};

#endif
