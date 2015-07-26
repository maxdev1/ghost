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

#include <components/text/TextField.hpp>
#include <components/text/move/DefaultCursorMoveStrategy.hpp>
#include <events/KeyEvent.hpp>
#include <events/MouseEvent.hpp>
#include <events/FocusEvent.hpp>
#include <Cursor.hpp>
#include <Fonts.hpp>

#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/utils/logger.hpp>
#include <sstream>

/**
 *
 */
TextField::TextField() :
		cursor(0), scrollX(0), focused(false), visualStatus(TextFieldVisualStatus::NORMAL), fontSize(10), textColor(RGB(0, 0, 0)), insets(g_insets(5, 5, 5, 5)) {
	// Dummy values, if someone forgets to properly setFont
	heightOfCapitalX = 10;
	lineHeight = 10;
	cursorMoveStrategy = DefaultCursorMoveStrategy::getInstance();

	setFont(Fonts::getDefault());
}

/**
 *
 */
TextField::~TextField() {

}

/**
 *
 */
void TextField::setText(std::string newText) {
	text = newText;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

/**
 *
 */
void TextField::update() {
	// Perform layouting
	g_rectangle bounds = getBounds();
	viewModel = g_layouted_text();
	g_text_layouter::getInstance()->layout(text, font, fontSize, g_rectangle(0, 0, bounds.width, bounds.height), g_text_alignment::LEFT, viewModel, false);
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void TextField::paint() {

	if (font == 0) {
		return;
	}

	g_rectangle bounds = getBounds();
	g_painter p(graphics);

	// Background
	p.setColor(RGB(255, 255, 255));
	p.fill(g_rectangle(0, 0, bounds.width, bounds.height));

	// Border
	if (focused) {
		p.setColor(RGB(55, 155, 255));
	} else if (visualStatus == TextFieldVisualStatus::HOVERED) {
		p.setColor(RGB(150, 150, 150));
	} else {
		p.setColor(RGB(180, 180, 180));
	}
	p.draw(g_rectangle(0, 0, bounds.width - 1, bounds.height - 1));

	// Scroll
	applyScroll();

	int pos = 0;
	int first = marker < cursor ? marker : cursor;
	int second = marker > cursor ? marker : cursor;

	// Paint marking
	if (focused) {
		pos = 0;
		for (g_positioned_glyph& g : viewModel.positions) {

			g_color_argb color = textColor;
			if (first != second && pos >= first && pos < second) {
				p.setColor(RGB(55, 155, 255));
				g_rectangle before = positionToCursorBounds(pos);
				g_rectangle after = positionToCursorBounds(pos + 1);
				p.fill(g_rectangle(before.x, before.y, after.x - before.x, before.height));

				color = RGB(255, 255, 255);
			}
			++pos;
		}
	}

	// Paint glyphs
	pos = 0;
	for (g_positioned_glyph& g : viewModel.positions) {

		g_rectangle onView = glyphToView(g);
		g_color_argb color = textColor;
		if (focused && first != second && pos >= first && pos < second) {
			color = RGB(255, 255, 255);
		}

		p.drawColoredBitmap(onView.x, onView.y, g.glyph->getBitmap(), color, onView.width, onView.height);
		++pos;
	}

	// Paint cursor
	if (focused) {
		p.setColor(RGB(60, 60, 60));
		p.fill(positionToCursorBounds(cursor));
	}
}

/**
 *
 */
void TextField::applyScroll() {

	int cursorX = positionToUnscrolledCursorX(cursor);
	g_rectangle bounds = getBounds();

	// Scroll
	if (scrollX + cursorX > bounds.width - insets.right) {
		scrollX = bounds.width - cursorX - insets.right;
	} else if (scrollX + cursorX < insets.left) {
		scrollX = -cursorX + insets.left; // TODO ?? + bounds.width / 2;
	}
}

/**
 *
 */
int TextField::positionToUnscrolledCursorX(int pos) {

	int cursorX = insets.left;
	int positionsCount = viewModel.positions.size();
	for (int i = 0; i < positionsCount; i++) {
		g_positioned_glyph& g = viewModel.positions[i];
		// After last?
		if (i == positionsCount - 1 && pos == positionsCount) {
			cursorX = g.position.x + insets.left + g.glyph->getAdvance().x;
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
void TextField::setCursor(int pos) {
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
void TextField::setMarker(int pos) {
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
void TextField::backspace(g_key_info& info) {

	if (text.length() > 0) {

		g_range selected = getSelectedRange();

		int leftcut = selected.getFirst();
		if (info.alt) {
			leftcut = cursorMoveStrategy->calculateSkip(text, leftcut, CursorDirection::LEFT);
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
void TextField::insert(std::string ins) {

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
bool TextField::handle(Event& e) {

	static bool shiftDown = false;

	KeyEvent* ke = dynamic_cast<KeyEvent*>(&e);
	if (ke) {
		if (ke->info.key == "KEY_SHIFT_L") {
			shiftDown = ke->info.pressed;
		}

		if (ke->info.pressed) {

			if (ke->info.key == "KEY_BACKSPACE") {
				backspace(ke->info);

			} else if (ke->info.key == "KEY_ARROW_LEFT") {
				cursorMoveStrategy->moveCursor(this, CursorDirection::LEFT, ke->info);

			} else if (ke->info.key == "KEY_ARROW_RIGHT") {
				cursorMoveStrategy->moveCursor(this, CursorDirection::RIGHT, ke->info);

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

	FocusEvent* fe = dynamic_cast<FocusEvent*>(&e);
	if (fe) {
		if (fe->type == FOCUS_EVENT_GAINED) {
			focused = true;
		} else {
			focused = false;
		}
		markFor(COMPONENT_REQUIREMENT_PAINT);
		return true;
	}

	MouseEvent* me = dynamic_cast<MouseEvent*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			visualStatus = TextFieldVisualStatus::HOVERED;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			Cursor::set("text");

		} else if (me->type == MOUSE_EVENT_LEAVE) {
			visualStatus = TextFieldVisualStatus::NORMAL;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			Cursor::set("default");

		} else if (me->type == MOUSE_EVENT_PRESS) {

			g_point p = me->position;
			int clickCursor = viewToPosition(p);

			if (me->clickCount > 2) {
				marker = 0;
				cursor = text.length();

			} else if (me->clickCount == 2) {
				marker = cursorMoveStrategy->calculateSkip(text, clickCursor, CursorDirection::LEFT);
				cursor = cursorMoveStrategy->calculateSkip(text, clickCursor, CursorDirection::RIGHT);

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
int TextField::viewToPosition(g_point p) {

	int pos = 0;

	for (int i = 0; i < viewModel.positions.size(); i++) {
		g_positioned_glyph g = viewModel.positions[i];
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
g_rectangle TextField::glyphToView(g_positioned_glyph g) {

	int yOffset = getBounds().height / 2 - heightOfCapitalX - 4;
	int x = scrollX + g.position.x + insets.left;
	int y = g.position.y + yOffset;
	g_dimension bitmapDimension = g.glyph->getBitmapSize();
	return g_rectangle(x, y, bitmapDimension.width, bitmapDimension.height);
}

/**
 *
 */
g_rectangle TextField::positionToCursorBounds(int pos) {

	int cursorX = positionToUnscrolledCursorX(pos);
	return g_rectangle(scrollX + cursorX, getBounds().height / 2 - lineHeight / 2, 1, lineHeight);
}

/**
 *
 */
void TextField::setFont(g_font* f) {
	font = f;

	g_glyph* glyphCapitalX = font->getGlyph(fontSize, 'X');
	heightOfCapitalX = glyphCapitalX->getBounding().height;
	lineHeight = font->getLineHeight(fontSize);
}

/**
 *
 */
g_range TextField::getSelectedRange() {
	return g_range(marker, cursor);
}
