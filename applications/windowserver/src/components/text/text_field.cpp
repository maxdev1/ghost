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

#include <components/cursor.hpp>
#include <components/text/move/default_caret_move_strategy.hpp>
#include <components/text/text_field.hpp>
#include <events/focus_event.hpp>
#include <events/key_event.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/text/font_loader.hpp>
#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/ui/properties.hpp>
#include <ghostuser/utils/logger.hpp>
#include <sstream>

/**
 *
 */
text_field_t::text_field_t() :
		cursor(0), marker(0), scrollX(0), secure(false), focused(false), visualStatus(text_field_visual_status_t::NORMAL), fontSize(14), textColor(
				RGB(0, 0, 0)), insets(g_insets(5, 5, 5, 5)) {
	caretMoveStrategy = default_caret_move_strategy_t::getInstance();

	viewModel = g_text_layouter::getInstance()->initializeBuffer();
	setFont(g_font_loader::getDefault());
}

/**
 *
 */
text_field_t::~text_field_t() {

}

/**
 *
 */
void text_field_t::setText(std::string newText) {
	text = newText;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
void text_field_t::update() {

	// Perform layouting
	g_rectangle bounds = getBounds();

	std::string visible_text = text;
	if (secure) {
		visible_text = "";
		for (int i = 0; i < text.length(); i++) {
			visible_text += "*";
		}
	}

	g_text_layouter::getInstance()->layout(graphics.getContext(), visible_text.c_str(), font, fontSize, g_rectangle(0, 0, bounds.width, bounds.height),
			g_text_alignment::LEFT, viewModel, false);
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void text_field_t::paint() {

	if (font == 0) {
		return;
	}

	auto cr = graphics.getContext();
	g_rectangle bounds = getBounds();

	// background
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_fill(cr);

	// border
	g_color_argb borderColor;
	if (focused) {
		borderColor = RGB(55, 155, 255);
	} else if (visualStatus == text_field_visual_status_t::HOVERED) {
		borderColor = RGB(150, 150, 150);
	} else {
		borderColor = RGB(180, 180, 180);
	}
	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(borderColor));
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_stroke(cr);

	// Scroll
	applyScroll();

	int pos = 0;
	int first = marker < cursor ? marker : cursor;
	int second = marker > cursor ? marker : cursor;

	// Paint marking
	if (focused) {
		pos = 0;
		for (g_positioned_glyph& g : viewModel->positions) {

			g_color_argb color = textColor;
			if (first != second && pos >= first && pos < second) {
				cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(RGB(55,155,255)));
				g_rectangle before = positionToCursorBounds(pos);
				g_rectangle after = positionToCursorBounds(pos + 1);
				cairo_rectangle(cr, before.x, before.y, after.x - before.x, before.height);
				cairo_fill(cr);

				color = RGB(255, 255, 255);
			}
			++pos;
		}
	}

	// Paint glyphs
	pos = 0;
	for (g_positioned_glyph& g : viewModel->positions) {

		g_rectangle onView = glyphToView(g);
		g_color_argb color = textColor;
		if (focused && first != second && pos >= first && pos < second) {
			color = RGB(255, 255, 255);
		}

		cairo_save(cr);
		cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(color));
		cairo_translate(cr, onView.x - g.glyph->x, onView.y - g.glyph->y); // TODO?
		cairo_glyph_path(cr, g.glyph, g.glyph_count);
		cairo_fill(cr);
		cairo_restore(cr);
		++pos;
	}

	// Paint cursor
	if (focused) {
		cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(RGB(60, 60, 60)));
		auto bounds = positionToCursorBounds(cursor);
		cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
		cairo_fill(cr);
	}
}

/**
 *
 */
void text_field_t::applyScroll() {

	int cursorX = positionToUnscrolledCursorX(cursor);
	g_rectangle bounds = getBounds();

	// scroll
	if (scrollX + cursorX > bounds.width - insets.right) {
		scrollX = bounds.width - cursorX - insets.right;
	} else if (scrollX + cursorX < insets.left) {
		scrollX = -cursorX + insets.left; // TODO ?? + bounds.width / 2;
	}
}

/**
 *
 */
int text_field_t::positionToUnscrolledCursorX(int pos) {

	int cursorX = insets.left;
	int positionsCount = viewModel->positions.size();
	for (int i = 0; i < positionsCount; i++) {
		g_positioned_glyph& g = viewModel->positions[i];
		// After last?
		if (i == positionsCount - 1 && pos == positionsCount) {
			cursorX = g.position.x + insets.left + g.advance.x;
		}
		// Anywhere inside
		if (i == pos) {
			cursorX = g.position.x + insets.left - 1;
		}
	}

	return cursorX;
}

/**
 *
 */
void text_field_t::setCursor(int pos) {
	cursor = pos;
	if (cursor < 0) {
		cursor = 0;
	}
	if (cursor > text.length()) {
		cursor = text.length();
	}

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void text_field_t::setMarker(int pos) {
	marker = pos;
	if (marker < 0) {
		marker = 0;
	}
	if (marker > text.length()) {
		marker = text.length();
	}

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void text_field_t::backspace(g_key_info& info) {

	if (text.length() > 0) {

		g_range selected = getSelectedRange();

		int leftcut = selected.getFirst();
		if (info.alt) {
			leftcut = caretMoveStrategy->calculateSkip(text, leftcut, caret_direction_t::LEFT);
		} else if (info.ctrl) {
			leftcut = 0;
		}

		int rightcut = selected.getLast();

		if (rightcut - leftcut == 0) {
			leftcut--;
		}

		if (leftcut >= 0 && rightcut <= text.length()) {
			std::string beforeCursor = text.substr(0, leftcut);
			std::string afterCursor = text.substr(rightcut);
			text = beforeCursor + afterCursor;
			setCursor(leftcut);
			setMarker(leftcut);

			markFor(COMPONENT_REQUIREMENT_UPDATE);
		}
	}

}

/**
 *
 */
void text_field_t::insert(std::string ins) {

	g_range selected = getSelectedRange();

	int first = selected.getFirst();
	int last = selected.getLast();

	if (first < 0) {
		first = 0;
	}
	if (last > text.size()) {
		last = text.size();
	}

	std::string beforeCursor = text.substr(0, first);
	std::string afterCursor = text.substr(last);

	text = beforeCursor + ins + afterCursor;
	setCursor(first + ins.length());
	setMarker(first + ins.length());
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
bool text_field_t::handle(event_t& e) {

	static bool shiftDown = false;

	key_event_t* ke = dynamic_cast<key_event_t*>(&e);
	if (ke) {
		if (ke->info.key == "KEY_SHIFT_L") {
			shiftDown = ke->info.pressed;
		}

		if (ke->info.pressed) {

			if (ke->info.key == "KEY_BACKSPACE") {
				backspace(ke->info);

			} else if (ke->info.key == "KEY_ARROW_LEFT") {
				caretMoveStrategy->moveCaret(this, caret_direction_t::LEFT, ke->info);

			} else if (ke->info.key == "KEY_ARROW_RIGHT") {
				caretMoveStrategy->moveCaret(this, caret_direction_t::RIGHT, ke->info);

			} else if (ke->info.key == "KEY_A" && ke->info.ctrl) {
				marker = 0;
				cursor = text.length();
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else {
				char c = g_keyboard::charForKey(ke->info);

				if (c != -1) {
					std::stringstream s;
					s << c;
					insert(s.str());
				}
			}
		}
		return true;
	}

	focus_event_t* fe = dynamic_cast<focus_event_t*>(&e);
	if (fe) {
		if (fe->type == FOCUS_EVENT_GAINED) {
			focused = true;
		} else {
			focused = false;
		}
		markFor(COMPONENT_REQUIREMENT_PAINT);
		return true;
	}

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			visualStatus = text_field_visual_status_t::HOVERED;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			cursor_t::set("text");

		} else if (me->type == MOUSE_EVENT_LEAVE) {
			visualStatus = text_field_visual_status_t::NORMAL;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			cursor_t::set("default");

		} else if (me->type == MOUSE_EVENT_PRESS) {

			g_point p = me->position;
			int clickCursor = viewToPosition(p);

			if (me->clickCount > 2) {
				marker = 0;
				cursor = text.length();

			} else if (me->clickCount == 2) {
				marker = caretMoveStrategy->calculateSkip(text, clickCursor, caret_direction_t::LEFT);
				cursor = caretMoveStrategy->calculateSkip(text, clickCursor, caret_direction_t::RIGHT);

			} else {
				cursor = clickCursor;
				if (!shiftDown) {
					marker = cursor;
				}
			}

			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_DRAG) {
			g_point p = me->position;
			cursor = viewToPosition(p);
			markFor(COMPONENT_REQUIREMENT_PAINT);
		}

		return true;
	}

	return false;
}

/**
 *
 */
int text_field_t::viewToPosition(g_point p) {

	int pos = 0;

	for (int i = 0; i < viewModel->positions.size(); i++) {
		g_positioned_glyph g = viewModel->positions[i];
		g_rectangle onView = glyphToView(g);

		if (p.x < onView.x + onView.width / 2) {
			break;
		}

		++pos;
	}
	return pos;
}

/**
 *
 */
g_rectangle text_field_t::glyphToView(g_positioned_glyph& g) {

	int yOffset = getBounds().height / 2 - fontSize / 2 - 2;
	int x = scrollX + g.position.x + insets.left;
	int y = g.position.y + yOffset;
	return g_rectangle(x, y, g.size.width, g.size.height);
}

/**
 *
 */
g_rectangle text_field_t::positionToCursorBounds(int pos) {

	int cursorX = positionToUnscrolledCursorX(pos);
	int caretHeight = fontSize + 4;
	return g_rectangle(scrollX + cursorX, getBounds().height / 2 - caretHeight / 2, 1, caretHeight);
}

/**
 *
 */
void text_field_t::setFont(g_font* f) {
	font = f;
}

/**
 *
 */
g_range text_field_t::getSelectedRange() {
	return g_range(marker, cursor);
}

/**
 *
 */
bool text_field_t::getBoolProperty(int property, bool* out) {

	if (property == G_UI_PROPERTY_SECURE) {
		*out = secure;
		return true;
	}

	return false;
}

/**
 *
 */
bool text_field_t::setBoolProperty(int property, bool value) {

	if (property == G_UI_PROPERTY_SECURE) {
		secure = value;
		return true;
	}

	return false;
}
