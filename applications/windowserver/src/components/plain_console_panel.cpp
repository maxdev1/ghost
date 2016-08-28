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

#include <components/plain_console_panel.hpp>
#include <events/focus_event.hpp>
#include <ghostuser/graphics/text/font_loader.hpp>

/**
 *
 */
plain_console_panel_t::plain_console_panel_t() {
	font = g_font_loader::getDefault();
	focused = false;
}

/**
 *
 */
void plain_console_panel_t::update() {

	g_rectangle thisBounds(0, 0, getBounds().width, getBounds().height);

	// Check if the component was ever layouted, otherwise set to a high value
	if (thisBounds.width == 0 && thisBounds.height == 0) {
		thisBounds.width = 9999;
		thisBounds.height = 9999;
	}

	// Reset view model, layout characters
	// TODO
/*	viewModel = g_layouted_text();
	g_text_layouter::getInstance()->layout(content, font, 10, thisBounds, g_text_alignment::LEFT, viewModel);
	g_dimension newPreferred(viewModel.textBounds.width + 3, viewModel.textBounds.height + 3);

	// Set new preferred size
	if (getPreferredSize() != newPreferred) {
		setPreferredSize(newPreferred);
	}*/

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void plain_console_panel_t::paint() {

	// TODO
	/*g_painter p(graphics);
	p.setColor(RGB(255, 255, 255));
	p.fill(g_rectangle(0, 0, getBounds().width, getBounds().height));

	if (focused) {
		p.setColor(RGB(80, 140, 255));
	} else {
		p.setColor(RGB(200, 200, 200));
	}
	p.draw(g_rectangle(0, 0, getBounds().width - 1, getBounds().height - 1));

	for (g_positioned_glyph& glyph : viewModel.positions) {
		g_glyph* current = glyph.glyph;
		g_dimension bitmapSize = current->getBitmapSize();
		p.drawColoredBitmap(glyph.position.x, getBounds().height - viewModel.textBounds.height + glyph.position.y, current->getBitmap(), RGB(0, 0, 0),
				bitmapSize.width, bitmapSize.height);
	}*/
}

/**
 *
 */
bool plain_console_panel_t::handle(event_t& e) {

	focus_event_t* fe = dynamic_cast<focus_event_t*>(&e);
	if (fe) {
		if (fe->type == FOCUS_EVENT_GAINED) {
			focused = true;
		} else if (fe->type == FOCUS_EVENT_LOST) {
			focused = false;
		}
		markFor(COMPONENT_REQUIREMENT_PAINT);

		return true;
	}

	return false;
}

/**
 *
 */
void plain_console_panel_t::append(char c) {
	content += c;
	if (content.length() > 100) {
		content = content.substr(content.length() - 100);
	}
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}
