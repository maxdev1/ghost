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

#include <components/button.hpp>
#include <components/window.hpp>

#include <events/focus_event.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <ghostuser/utils/Logger.hpp>

/**
 *
 */
button_t::button_t() :
		insets(g_insets(5, 5, 5, 5)) {
	addChild(&label);
	label.setAlignment(g_text_alignment::CENTER);
}

/**
 *
 */
void button_t::layout() {
	g_dimension labelPreferred = label.getPreferredSize();

	labelPreferred.width += insets.left + insets.right;
	labelPreferred.height += insets.top + insets.bottom;
	setPreferredSize(labelPreferred);
}

/**
 *
 */
void button_t::paint() {

	g_painter p(graphics);
	p.setColor(state.pressed ? RGB(230, 230, 230) : (state.hovered ? RGB(250, 250, 250) : RGB(240, 240, 240)));
	p.fill(g_rectangle(0, 0, getBounds().width, getBounds().height));

	if (state.focused) {
		p.setColor(RGB(55, 155, 255));
	} else {
		p.setColor(RGB(180, 180, 180));
	}
	p.draw(g_rectangle(0, 0, getBounds().width - 1, getBounds().height - 1));
}

/**
 *
 */
bool button_t::handle(event_t& e) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			state.hovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_LEAVE) {
			state.hovered = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_PRESS) {
			state.pressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		} else if (me->type == MOUSE_EVENT_RELEASE || me->type == MOUSE_EVENT_DRAG_RELEASE) {
			state.pressed = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);

			if (me->type == MOUSE_EVENT_RELEASE) {
				if (me->position.x >= 0 && me->position.y >= 0 && me->position.x < getBounds().width && me->position.y < getBounds().height) {
					fireAction();
				}
			}
		}
		return true;
	}

	focus_event_t* fe = dynamic_cast<focus_event_t*>(&e);
	if (fe) {
		if (fe->type == FOCUS_EVENT_GAINED) {
			state.focused = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return true;
		} else if (fe->type == FOCUS_EVENT_LOST) {
			state.focused = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return true;
		}
	}

	return false;
}

/**
 *
 */
void button_t::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle labelBounds = getBounds();
	labelBounds.x = insets.left;
	labelBounds.y = insets.right;
	this->label.setBounds(labelBounds);
}

/**
 *
 */
void button_t::setTitle(std::string title) {
	this->label.setTitle(title);
}

/**
 *
 */
std::string button_t::getTitle() {
	return this->label.getTitle();
}
