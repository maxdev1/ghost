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

#include "components/text/text_area.hpp"
#include "components/cursor.hpp"
#include "components/text/move/default_caret_move_strategy.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"

#include <libproperties/parser.hpp>
#include <libwindow/properties.hpp>
#include <libfont/font_loader.hpp>
#include <sstream>


// TODO all work in progress, will be unified with text_field once it's clear what differences are necessary

text_area_t::text_area_t() :
	text(""), cursor(0), marker(0), scrollX(0), secure(false),
	visualStatus(text_area_visual_status_t::NORMAL), fontSize(14),
	textColor(RGB(0, 0, 0)), insets(g_insets(5, 5, 5, 5))
{
	caretMoveStrategy = default_caret_move_strategy_t::getInstance();
	viewModel = g_text_layouter::getInstance()->initializeBuffer();
	font = g_font_loader::getDefault();
}

void text_area_t::setText(std::string newText)
{
	text = newText;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

std::string text_area_t::getText()
{
	return text;
}

void text_area_t::setSecure(bool newSecure)
{
	secure = newSecure;
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

void text_area_t::update()
{
	g_rectangle bounds = getBounds();

	std::string visible_text = text;
	if(secure)
	{
		visible_text = "";
		for(int i = 0; i < text.length(); i++)
		{
			visible_text += "*";
		}
	}

	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	g_text_layouter::getInstance()->layout(cr, visible_text.c_str(), font, fontSize,
	                                       g_rectangle(0, 0, bounds.width, bounds.height), g_text_alignment::LEFT,
	                                       viewModel, true);

	markFor(COMPONENT_REQUIREMENT_PAINT);

	graphics.releaseContext();
}

void text_area_t::paint()
{
	if(font == nullptr)
		return;

	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	g_rectangle bounds = getBounds();

	// background
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_fill(cr);

	// border
	g_color_argb borderColor;
	if(focused)
	{
		borderColor = RGB(55, 155, 255);
	}
	else if(visualStatus == text_area_visual_status_t::HOVERED)
	{
		borderColor = RGB(150, 150, 150);
	}
	else
	{
		borderColor = RGB(180, 180, 180);
	}
	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(borderColor));
	cairo_rectangle(cr, 0.5, 0.5, bounds.width - 1, bounds.height - 1);
	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);

	// TODO Unfortunately need to do this again because we have a new cairo context otherwise they are lost
	std::string visible_text = text;
	if(secure)
	{
		visible_text = "";
		for(int i = 0; i < text.length(); i++)
		{
			visible_text += "*";
		}
	}
	g_text_layouter::getInstance()->layout(cr, visible_text.c_str(), font, fontSize,
	                                       g_rectangle(0, 0, bounds.width, bounds.height), g_text_alignment::LEFT,
	                                       viewModel, true);

	// Scroll
	applyScroll();

	int pos = 0;
	int first = marker < cursor ? marker : cursor;
	int second = marker > cursor ? marker : cursor;

	// Paint marking
	if(focused)
	{
		pos = 0;
		for(pos = 0; pos < viewModel->positions.size(); pos++)
		{
			g_color_argb color = textColor;
			if(first != second && pos >= first && pos < second)
			{
				cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(RGB(55, 155, 255)));
				g_rectangle before = positionToCursorBounds(pos);
				g_rectangle after = positionToCursorBounds(pos + 1);
				cairo_rectangle(cr, before.x, before.y, after.x - before.x, before.height);
				cairo_fill(cr);

				color = RGB(255, 255, 255);
			}
		}
	}

	// Paint glyphs
	pos = 0;
	for(g_positioned_glyph& g: viewModel->positions)
	{

		g_rectangle onView = glyphToView(g);
		g_color_argb color = textColor;
		if(focused && first != second && pos >= first && pos < second)
		{
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
	if(focused)
	{
		cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(RGB(60, 60, 60)));
		auto cursorBounds = positionToCursorBounds(cursor);
		cairo_rectangle(cr, cursorBounds.x, cursorBounds.y, cursorBounds.width, cursorBounds.height);
		cairo_fill(cr);
	}

	graphics.releaseContext();
}

void text_area_t::applyScroll()
{
	auto cursorPos = positionToUnscrolledCursorPoint(cursor);
	g_rectangle bounds = getBounds();

	int textUsableWidth = bounds.width - insets.left - insets.right;

	if(scrollX + cursorPos.x > textUsableWidth)
		scrollX = textUsableWidth - cursorPos.x;
	else if(scrollX + cursorPos.x < insets.left)
		scrollX = -cursorPos.x + insets.left;

	if(viewModel->textBounds.width > textUsableWidth)
		scrollX = std::max(textUsableWidth - viewModel->textBounds.width,
		                   std::min(scrollX, 0));
	else
		scrollX = 0;
}

g_point text_area_t::positionToUnscrolledCursorPoint(int pos)
{
	int cursorX = insets.left;
	int cursorY = insets.top;

	int positionsCount = viewModel->positions.size();
	for(int i = 0; i < positionsCount; i++)
	{
		g_positioned_glyph& g = viewModel->positions[i];
		// After last?
		if(i == positionsCount - 1 && pos == positionsCount)
		{
			cursorX = g.position.x + insets.left + g.advance.x;
			cursorY = g.position.y + insets.top + g.advance.y;
		}
		// Anywhere inside
		if(i == pos)
		{
			cursorX = g.position.x + insets.left - 1;
			cursorY = g.position.y + insets.top + g.advance.y;
		}
	}

	g_point cursorPoint;
	cursorPoint.x = cursorX;
	cursorPoint.y = cursorY;
	return cursorPoint;
}

void text_area_t::setCursor(int pos)
{
	cursor = pos;
	if(cursor < 0)
	{
		cursor = 0;
	}
	if(cursor > text.length())
	{
		cursor = text.length();
	}

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

void text_area_t::setMarker(int pos)
{
	marker = pos;
	if(marker < 0)
	{
		marker = 0;
	}
	if(marker > text.length())
	{
		marker = text.length();
	}

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

void text_area_t::backspace(g_key_info& info)
{

	if(text.length() > 0)
	{

		g_range selected = getSelectedRange();

		int leftcut = selected.getFirst();
		if(info.alt)
		{
			leftcut = caretMoveStrategy->calculateSkip(text, leftcut, caret_direction_t::LEFT);
		}
		else if(info.ctrl)
		{
			leftcut = 0;
		}

		int rightcut = selected.getLast();

		if(rightcut - leftcut == 0)
		{
			leftcut--;
		}

		if(leftcut >= 0 && rightcut <= text.length())
		{
			std::string beforeCursor = text.substr(0, leftcut);
			std::string afterCursor = text.substr(rightcut);
			text = beforeCursor + afterCursor;
			setCursor(leftcut);
			setMarker(leftcut);

			markFor(COMPONENT_REQUIREMENT_UPDATE);
		}
	}
}

void text_area_t::insert(std::string ins)
{

	g_range selected = getSelectedRange();

	int first = selected.getFirst();
	int last = selected.getLast();

	if(first < 0)
	{
		first = 0;
	}
	if(last > text.size())
	{
		last = text.size();
	}

	std::string beforeCursor = text.substr(0, first);
	std::string afterCursor = text.substr(last);

	text = beforeCursor + ins + afterCursor;
	setCursor(first + ins.length());
	setMarker(first + ins.length());
	markFor(COMPONENT_REQUIREMENT_UPDATE);
}

component_t* text_area_t::handleKeyEvent(key_event_t& ke)
{
	if(ke.info.key == "KEY_SHIFT_L")
	{
		shiftDown = ke.info.pressed;
	}

	if(ke.info.pressed)
	{
		if(ke.info.key == "KEY_BACKSPACE")
		{
			backspace(ke.info);
		}
		else if(ke.info.key == "KEY_ARROW_LEFT")
		{
			caretMoveStrategy->moveCaret(this, caret_direction_t::LEFT, ke.info);
		}
		else if(ke.info.key == "KEY_ARROW_RIGHT")
		{
			caretMoveStrategy->moveCaret(this, caret_direction_t::RIGHT, ke.info);
		}
		else if(ke.info.key == "KEY_A" && ke.info.ctrl)
		{
			marker = 0;
			cursor = text.length();
			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		else
		{
			char c = platformCharForKey(ke.info);

			if(c != -1)
			{
				std::stringstream s;
				s << c;
				insert(s.str());
			}
		}
	}
	sendKeyEventToListener(ke);
	return this;
}

void text_area_t::setFocusedInternal(bool focused)
{
	focusable_component_t::setFocusedInternal(focused);
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

component_t* text_area_t::handleMouseEvent(mouse_event_t& me)
{
	if(me.type == G_MOUSE_EVENT_ENTER)
	{
		visualStatus = text_area_visual_status_t::HOVERED;
		markFor(COMPONENT_REQUIREMENT_PAINT);
		cursor_t::set("text");
	}
	else if(me.type == G_MOUSE_EVENT_LEAVE)
	{
		visualStatus = text_area_visual_status_t::NORMAL;
		markFor(COMPONENT_REQUIREMENT_PAINT);
		cursor_t::set("default");
	}
	else if(me.type == G_MOUSE_EVENT_PRESS)
	{

		g_point p = me.position;
		int clickCursor = viewToPosition(p);

		if(me.clickCount > 2)
		{
			marker = 0;
			cursor = text.length();
		}
		else if(me.clickCount == 2)
		{
			marker = caretMoveStrategy->calculateSkip(text, clickCursor, caret_direction_t::LEFT);
			cursor = caretMoveStrategy->calculateSkip(text, clickCursor, caret_direction_t::RIGHT);
		}
		else
		{
			cursor = clickCursor;
			if(!shiftDown)
			{
				marker = cursor;
			}
		}

		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_DRAG)
	{
		g_point p = me.position;
		cursor = viewToPosition(p);
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}

	return this;
}

int text_area_t::viewToPosition(g_point p)
{
	int pos = 0;

	for(int i = 0; i < viewModel->positions.size(); i++)
	{
		g_positioned_glyph g = viewModel->positions[i];
		g_rectangle onView = glyphToView(g);

		if(p.x < onView.x + onView.width / 2
		   // TODO only multiline
		   && p.y + fontSize / 2 > onView.y
		   && p.y + fontSize / 2 < onView.y + onView.height / 2
		)
		{
			break;
		}

		++pos;
	}
	return pos;
}

g_rectangle text_area_t::glyphToView(g_positioned_glyph& g)
{
	int yOffset = getBounds().height / 2 - fontSize / 2 - 2;
	int x = scrollX + g.position.x + insets.left;
	int y = g.position.y + insets.top; // TODO single-line must use yOffset
	return g_rectangle(x, y, g.size.width, g.size.height);
}

g_rectangle text_area_t::positionToCursorBounds(int pos)
{
	auto cursorPoint = positionToUnscrolledCursorPoint(pos);
	int caretHeight = fontSize + 4;

	int yOffset = getBounds().height / 2 - caretHeight / 2;

	return g_rectangle(
		cursorPoint.x + scrollX,
		cursorPoint.y - caretHeight+ insets.top, // TODO single line must use + yOffset,
		1,
		caretHeight);
}

void text_area_t::setFont(g_font* f)
{
	font = f;
}

g_range text_area_t::getSelectedRange()
{
	return g_range(marker, cursor);
}

bool text_area_t::getNumericProperty(int property, uint32_t* out)
{
	if(property == G_UI_PROPERTY_SECURE)
	{
		*out = secure;
		return true;
	}

	return false;
}

bool text_area_t::setNumericProperty(int property, uint32_t value)
{
	if(property == G_UI_PROPERTY_SECURE)
	{
		secure = value;
		return true;
	}

	return component_t::setNumericProperty(property, value);
}
