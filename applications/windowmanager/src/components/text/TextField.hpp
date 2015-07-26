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

#ifndef TEXTFIELD_HPP_
#define TEXTFIELD_HPP_

#include <components/text/TextComponent.hpp>

#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/io/keyboard.hpp>

#include <string>
#include <list>

/**
 *
 */
enum class TextFieldVisualStatus
	: uint8_t {
		NORMAL, HOVERED
};

/**
 *
 */
class TextField: public TextComponent {
private:
	std::string text;
	TextFieldVisualStatus visualStatus;
	bool focused;

	g_font* font;
	int heightOfCapitalX;
	int lineHeight;

	int scrollX;
	int fontSize;
	g_color_argb textColor;
	g_insets insets;

	int cursor;
	int marker;

	g_layouted_text viewModel;

	void loadDefaultFont();
	void applyScroll();

public:
	TextField();
	virtual ~TextField();

	/**
	 *
	 */
	virtual void update();

	/**
	 *
	 */
	virtual void paint();

	/**
	 *
	 */
	virtual bool handle(Event& e);

	/**
	 *
	 */
	virtual void setText(std::string text);

	/**
	 *
	 */
	virtual std::string getText() {
		return text;
	}

	/**
	 *
	 */
	virtual void setCursor(int pos);

	/**
	 *
	 */
	virtual int getCursor() {
		return cursor;
	}

	/**
	 *
	 */
	virtual void setMarker(int pos);

	/**
	 *
	 */
	virtual int getMarker() {
		return marker;
	}

	/**
	 *
	 */
	void backspace(g_key_info& info);

	/**
	 *
	 */
	void insert(std::string text);

	/**
	 *
	 */
	int viewToPosition(g_point p);

	/**
	 *
	 */
	g_rectangle glyphToView(g_positioned_glyph g);

	/**
	 *
	 */
	int positionToUnscrolledCursorX(int pos);

	/**
	 *
	 */
	g_rectangle positionToCursorBounds(int pos);

	/**
	 *
	 */
	void setFont(g_font* f);

	/**
	 *
	 */
	virtual g_range getSelectedRange();

};

#endif
