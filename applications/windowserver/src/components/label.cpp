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

#include <components/label.hpp>
#include <fonts.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/utils/Logger.hpp>

#include <sstream>
#include <fonts.hpp>
#include <ghost.h>

/**
 *
 */
label_t::label_t() :
		component_t(true) {
	font = Fonts::getDefault();
	fontSize = 10;
	alignment = g_text_alignment::LEFT;
	color = RGB(0, 0, 0);
}

/**
 *
 */
void label_t::update() {

	g_rectangle thisBounds(0, 0, getBounds().width, getBounds().height);

	// Check if the component was ever layouted, otherwise set to a high value
	if (thisBounds.width == 0 && thisBounds.height == 0) {
		thisBounds.width = 9999;
		thisBounds.height = 9999;
	}

	// Reset view model, layout characters
	viewModel = g_layouted_text();
	g_text_layouter::getInstance()->layout(text, font, fontSize, thisBounds, alignment, viewModel);
	g_dimension newPreferred(viewModel.textBounds.width + 3, viewModel.textBounds.height + 3);

	// Set new preferred size
	if (getPreferredSize() != newPreferred) {
		setPreferredSize(newPreferred);
	}

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void label_t::paint() {

	g_rectangle thisBounds(0, 0, getBounds().width, getBounds().height);

	graphics.clear();
	g_painter p(graphics);

	for (g_positioned_glyph& glyph : viewModel.positions) {
		g_glyph* current = glyph.glyph;
		g_dimension bitmapSize = current->getBitmapSize();
		p.drawColoredBitmap(glyph.position.x, glyph.position.y, current->getBitmap(), color, bitmapSize.width, bitmapSize.height);
	}
}

/**
 *
 */
bool label_t::handle(event_t& e) {

	return false;
}

/**
 *
 */
void label_t::setTitle(std::string newText) {
	text = newText;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
std::string label_t::getTitle() {
	return text;
}

/**
 *
 */
void label_t::setAlignment(g_text_alignment newAlignment) {
	alignment = newAlignment;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
g_text_alignment label_t::getAlignment() {
	return alignment;
}
